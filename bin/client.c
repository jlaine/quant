// SPDX-License-Identifier: BSD-2-Clause
//
// Copyright (c) 2016-2019, NetApp, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#define klib_unused

#include <http_parser.h>

#include <picoquic/h3zero.h>   // IWYU pragma: keep
#include <picoquic/picoquic.h> // IWYU pragma: keep

#include <picoquic/democlient.h>
#include <quant/quant.h>


#define timespec_to_double(diff)                                               \
    ((double)(diff).tv_sec + (double)(diff).tv_nsec / NS_PER_S)


#define bps(bytes, secs)                                                       \
    __extension__({                                                            \
        static char _str[32];                                                  \
        const double _bps =                                                    \
            (bytes) && (fpclassify(secs) != FP_ZERO) ? (bytes)*8 / (secs) : 0; \
        if (_bps > NS_PER_S)                                                   \
            snprintf(_str, sizeof(_str), "%.3f Gb/s", _bps / NS_PER_S);        \
        else if (_bps > US_PER_S)                                              \
            snprintf(_str, sizeof(_str), "%.3f Mb/s", _bps / US_PER_S);        \
        else if (_bps > MS_PER_S)                                              \
            snprintf(_str, sizeof(_str), "%.3f Kb/s", _bps / MS_PER_S);        \
        else                                                                   \
            snprintf(_str, sizeof(_str), "%.3f b/s", _bps);                    \
        _str;                                                                  \
    })


struct conn_cache_entry {
    struct sockaddr_in dst;
    struct q_conn * c;
#ifndef NO_MIGRATION
    bool rebound;
    uint8_t _unused[7];
#endif
};


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
KHASH_MAP_INIT_INT64(conn_cache, struct conn_cache_entry *)
#pragma clang diagnostic pop


static uint32_t timeout = 10;
static uint32_t num_bufs = 100000;
static uint32_t reps = 1;
static bool do_h3 = false;
static bool flip_keys = false;
static bool zlen_cids = false;
static bool write_files = false;
#ifndef NO_MIGRATION
static bool rebind = false;
static bool migrate = false;
#endif


struct stream_entry {
    sl_entry(stream_entry) next;
    struct q_conn * c;
    struct q_stream * s;
    char * url;
    struct timespec req_t;
    struct timespec rep_t;
    struct w_iov_sq req;
    struct w_iov_sq rep;
};


static sl_head(stream_list, stream_entry) sl = sl_head_initializer(sl);


static inline uint64_t __attribute__((nonnull))
conn_cache_key(const struct sockaddr * const sock)
{
    const struct sockaddr_in * const sock4 =
        (const struct sockaddr_in *)(const void *)sock;

    return ((uint64_t)sock4->sin_addr.s_addr
            << sizeof(sock4->sin_addr.s_addr) * 8) |
           (uint64_t)sock4->sin_port;
}


static void __attribute__((noreturn, nonnull)) usage(const char * const name,
                                                     const char * const ifname,
                                                     const char * const cache,
                                                     const char * const tls_log,
                                                     const char * const qlog,
                                                     const bool verify_certs)
{
    printf("%s [options] URL [URL...]\n", name);
    printf("\t[-i interface]\tinterface to run over; default %s\n", ifname);
    printf("\t[-q log]\twrite qlog events to file; default %s\n", qlog);
    printf("\t[-s cache]\tTLS 0-RTT state cache; default %s\n", cache);
    printf("\t[-l log]\tlog file for TLS keys; default %s\n", tls_log);
    printf("\t[-t timeout]\tidle timeout in seconds; default %u\n", timeout);
    printf("\t[-c]\t\tverify TLS certificates; default %s\n",
           verify_certs ? "true" : "false");
    printf("\t[-u]\t\tupdate TLS keys; default %s\n",
           flip_keys ? "true" : "false");
    printf("\t[-3]\t\tsend a static H3 request; default %s\n",
           do_h3 ? "true" : "false");
    printf("\t[-z]\t\tuse zero-length source connection IDs; default %s\n",
           zlen_cids ? "true" : "false");
    printf("\t[-w]\t\twrite retrieved objects to disk; default %s\n",
           write_files ? "true" : "false");
    printf("\t[-r reps]\trepetitions for all URLs; default %u\n", reps);
    printf("\t[-b bufs]\tnumber of network buffers to allocate; default %u\n",
           num_bufs);
#ifndef NO_MIGRATION
    printf("\t[-n]\t\tsimulate NAT rebind (use twice for \"real\" migration); "
           "default %s\n",
           rebind ? "true" : "false");
#endif
#ifndef NDEBUG
    printf("\t[-v verbosity]\tverbosity level (0-%d, default %d)\n", DLEVEL,
           util_dlevel);
#endif
    exit(0);
}


static void __attribute__((nonnull))
set_from_url(char * const var,
             const size_t len,
             const char * const url,
             const struct http_parser_url * const u,
             const enum http_parser_url_fields f,
             const char * const def)
{
    if ((u->field_set & (1 << f)) == 0) {
        strncpy(var, def, len);
        var[len - 1] = 0;
    } else {
        strncpy(var, &url[u->field_data[f].off], u->field_data[f].len);
        var[u->field_data[f].len] = 0;
    }
}


static struct q_conn * __attribute__((nonnull))
get(char * const url, struct w_engine * const w, khash_t(conn_cache) * cc)
{
    // parse and verify the URIs passed on the command line
    struct http_parser_url u = {0};
    if (http_parser_parse_url(url, strlen(url), 0, &u)) {
        warn(ERR, "http_parser_parse_url: %s",
             http_errno_description((enum http_errno)errno));
        return 0;
    }

    ensure((u.field_set & (1 << UF_USERINFO)) == 0 &&
               (u.field_set & (1 << UF_QUERY)) == 0 &&
               (u.field_set & (1 << UF_FRAGMENT)) == 0,
           "unsupported URL components");

    // extract relevant info from URL
    char dest[1024];
    char port[64];
    char path[2048];
    set_from_url(dest, sizeof(dest), url, &u, UF_HOST, "localhost");
    set_from_url(port, sizeof(port), url, &u, UF_PORT, "4433");
    set_from_url(path, sizeof(path), url, &u, UF_PATH, "/index.html");

    struct addrinfo * peer;
    const struct addrinfo hints = {.ai_family = PF_INET,
                                   .ai_socktype = SOCK_DGRAM,
                                   .ai_protocol = IPPROTO_UDP};
    const int err = getaddrinfo(dest, port, &hints, &peer);
    if (err) {
        warn(ERR, "getaddrinfo: %s", gai_strerror(err));
        freeaddrinfo(peer);
        return 0;
    }

    // add to stream list
    struct stream_entry * se = calloc(1, sizeof(*se));
    ensure(se, "calloc failed");
    sq_init(&se->rep);
    sl_insert_head(&sl, se, next);

    sq_init(&se->req);
    if (do_h3) {
        q_alloc(w, &se->req, 1024);
        struct w_iov * const v = sq_first(&se->req);
        size_t consumed;
        h3zero_client_create_stream_request(v->buf, v->len, (uint8_t *)path,
                                            strlen(path), 0, dest, &consumed);
        v->len = (uint16_t)consumed;
    } else {
        // assemble an HTTP/0.9 request
        char req_str[MAXPATHLEN + 6];
        const int req_str_len =
            snprintf(req_str, sizeof(req_str), "GET %s\r\n", path);
        q_chunk_str(w, req_str, (uint32_t)req_str_len, &se->req);
    }

    // do we have a connection open to this peer?
    khiter_t k = kh_get(conn_cache, cc, conn_cache_key(peer->ai_addr));
    struct conn_cache_entry * cce =
        (k == kh_end(cc) ? 0 : kh_val(cc, k)); // NOLINT
    const bool opened_new = cce == 0;
    if (cce == 0) {
        clock_gettime(CLOCK_MONOTONIC, &se->req_t);
        // no, open a new connection
        struct q_conn * const c = q_connect(
            w, peer->ai_addr, dest,
#ifndef NO_MIGRATION
            rebind ? 0 : &se->req, rebind ? 0 : &se->s,
#else
            0, 0,
#endif
            true,
            do_h3 ? "h3-" DRAFT_VERSION_STRING : "hq-" DRAFT_VERSION_STRING, 0);
        if (c == 0) {
            freeaddrinfo(peer);
            return 0;
        }

        if (do_h3) {
            // we need to open a uni stream for an empty H/3 SETTINGS frame
            struct q_stream * const ss = q_rsv_stream(c, false);
            if (ss == 0)
                return 0;
            static const uint8_t h3_empty_settings[] = {0x04, 0x00};
            // XXX lsquic doesn't like a FIN on this stream
            q_write_str(w, ss, (const char *)h3_empty_settings,
                        sizeof(h3_empty_settings), false);
        }

        cce = calloc(1, sizeof(*cce));
        ensure(cce, "calloc failed");
        cce->c = c;

        // insert into connection cache
        cce->dst = *(struct sockaddr_in *)&peer->ai_addr;
        int ret;
        k = kh_put(conn_cache, cc, conn_cache_key(peer->ai_addr), &ret);
        ensure(ret >= 1, "inserted returned %d", ret);
        kh_val(cc, k) = cce;
    }

    if (opened_new == false
#ifndef NO_MIGRATION
        || (rebind && cce->rebound == false)
#endif
    ) {
        se->s = q_rsv_stream(cce->c, true);
        if (se->s) {
            clock_gettime(CLOCK_MONOTONIC, &se->req_t);
            q_write(se->s, &se->req, true);
#ifndef NO_MIGRATION
            if (rebind && cce->rebound == false) {
                q_rebind_sock(cce->c, migrate);
                cce->rebound = true; // only rebind once
            }
#endif
        }
    }

    se->c = cce->c;
    se->url = url;
    freeaddrinfo(peer);
    return cce->c; // NOLINT
}


static void __attribute__((nonnull)) free_cc(khash_t(conn_cache) * cc)
{
    struct conn_cache_entry * cce;
    kh_foreach_value(cc, cce, { free(cce); });
    kh_destroy(conn_cache, cc);
}


static void free_se(struct stream_entry * const se)
{
    q_free(&se->req);
    q_free(&se->rep);
    free(se);
}


static void free_sl_head(void)
{
    struct stream_entry * const se = sl_first(&sl);
    sl_remove_head(&sl, next);
    free_se(se);
}


static void free_sl(void)
{
    while (sl_empty(&sl) == false)
        free_sl_head();
}


static void __attribute__((nonnull))
write_object(struct stream_entry * const se)
{
    char * const slash = strrchr(se->url, '/');
    if (slash && *(slash + 1) == 0)
        // this URL ends in a slash, so strip that to name the file
        *slash = 0;

    const int fd =
        open(*basename(se->url) == 0 ? "index.html" : basename(se->url),
             O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP);
    ensure(fd != -1, "cannot open %s", basename(se->url));

    struct iovec vec[IOV_MAX];
    struct w_iov * v = sq_first(&se->rep);
    int i = 0;
    while (v) {
        vec[i].iov_base = v->buf;
        vec[i].iov_len = v->len;
        if (++i == IOV_MAX || sq_next(v, next) == 0) {
            ensure(writev(fd, vec, i) != -1, "cannot writev");
            i = 0;
        }
        v = sq_next(v, next);
    }
    close(fd);
}


int main(int argc, char * argv[])
{
#ifndef NDEBUG
    util_dlevel = DLEVEL; // default to maximum compiled-in verbosity
#endif
    char ifname[IFNAMSIZ] = "lo"
#ifndef __linux__
                            "0"
#endif
        ;
    int ch;
    char cache[MAXPATHLEN] = "/tmp/" QUANT "-session";
    char tls_log[MAXPATHLEN] = "/tmp/" QUANT "-tlslog";
    char qlog[MAXPATHLEN] = "/tmp/" QUANT "-client.qlog";
    bool verify_certs = false;
    int ret = 0;

    while ((ch = getopt(argc, argv,
                        "hi:v:s:t:l:cu3zb:wr:q:"
#ifndef NO_MIGRATION
                        "n"
#endif
                        )) != -1) {
        switch (ch) {
        case 'i':
            strncpy(ifname, optarg, sizeof(ifname) - 1);
            break;
        case 's':
            strncpy(cache, optarg, sizeof(cache) - 1);
            break;
        case 'q':
            strncpy(qlog, optarg, sizeof(qlog) - 1);
            break;
        case 't':
            timeout = (uint32_t)MIN(600, strtoul(optarg, 0, 10)); // 10 min
            break;
        case 'b':
            num_bufs =
                (uint32_t)MAX(1000, MIN(strtoul(optarg, 0, 10), UINT32_MAX));
            break;
        case 'r':
            reps = (uint32_t)MAX(1, MIN(strtoul(optarg, 0, 10), UINT32_MAX));
            break;
        case 'l':
            strncpy(tls_log, optarg, sizeof(tls_log) - 1);
            break;
        case 'c':
            verify_certs = true;
            break;
        case 'u':
            flip_keys = true;
            break;
        case '3':
            do_h3 = true;
            break;
        case 'z':
            zlen_cids = true;
            break;
        case 'w':
            write_files = true;
            break;
#ifndef NO_MIGRATION
        case 'n':
            if (rebind)
                migrate = true;
            rebind = true;
            break;
#endif
        case 'v':
#ifndef NDEBUG
            util_dlevel = (short)MIN(DLEVEL, strtoul(optarg, 0, 10));
#endif
            break;
        case 'h':
        case '?':
        default:
            usage(basename(argv[0]), ifname, cache, tls_log, qlog,
                  verify_certs);
        }
    }

    struct w_engine * const w = q_init(
        ifname, &(const struct q_conf){
                    .conn_conf = &(
                        struct q_conn_conf){.enable_tls_key_updates = flip_keys,
                                            .enable_spinbit = true,
                                            .idle_timeout = timeout,
                                            .enable_zero_len_cid = zlen_cids},
                    .qlog = qlog,
                    .num_bufs = num_bufs,
                    .ticket_store = cache,
                    .tls_log = tls_log,
                    .enable_tls_cert_verify = verify_certs});
    khash_t(conn_cache) * cc = kh_init(conn_cache);

    if (reps > 1)
        puts("size\ttime\t\tbps\t\turl");
    for (uint64_t r = 1; r <= reps; r++) {
        int url_idx = optind;
        while (url_idx < argc) {
            // open a new connection, or get an open one
            warn(INF, "%s retrieving %s", basename(argv[0]), argv[url_idx]);
            get(argv[url_idx++], w, cc);
        }

        // collect the replies
        bool all_closed;
        do {
            all_closed = true;
            bool rxed_new = false;
            struct stream_entry * se = 0;
            struct stream_entry * tmp = 0;
            sl_foreach_safe (se, &sl, next, tmp) {
                if (se->c == 0 || se->s == 0 || q_is_conn_closed(se->c)) {
                    sl_remove(&sl, se, stream_entry, next);
                    free_se(se);
                    continue;
                }

                rxed_new |= q_read_stream(se->s, &se->rep, false);

                const bool is_closed = q_peer_closed_stream(se->s);
                all_closed &= is_closed;
                if (is_closed)
                    clock_gettime(CLOCK_MONOTONIC, &se->rep_t);
            }

            if (rxed_new == false) {
                struct q_conn * c;
                q_ready(w, timeout * NS_PER_S, &c);
                if (c == 0)
                    break;
            }

        } while (all_closed == false);

        // print/save the replies
        while (sl_empty(&sl) == false) {
            struct stream_entry * const se = sl_first(&sl);
            ret |= w_iov_sq_cnt(&se->rep) == 0;

            struct timespec diff;
            timespec_sub(&se->rep_t, &se->req_t, &diff);
            const double elapsed = timespec_to_double(diff);
            if (reps > 1)
                printf("%" PRIu "\t%f\t\"%s\"\t%s\n", w_iov_sq_len(&se->rep),
                       elapsed, bps(w_iov_sq_len(&se->rep), elapsed), se->url);
#ifndef NDEBUG
            char cid_str[64];
            q_cid(se->c, cid_str, sizeof(cid_str));
            warn(WRN,
                 "read %" PRIu
                 " byte%s in %.3f sec (%s) on conn %s strm %" PRIu,
                 w_iov_sq_len(&se->rep), plural(w_iov_sq_len(&se->rep)),
                 elapsed < 0 ? 0 : elapsed,
                 bps(w_iov_sq_len(&se->rep), elapsed), cid_str, q_sid(se->s));
#endif

            // retrieve the TX'ed request
            q_stream_get_written(se->s, &se->req);

            if (write_files)
                write_object(se);

            // save the object, and print its first three packets to stdout
            struct w_iov * v;
            uint32_t n = 0;
            sq_foreach (v, &se->rep, next) {
                const bool is_last = v == sq_last(&se->rep, w_iov, next);
                if (w_iov_sq_cnt(&se->rep) > 100 || reps > 1)
                    // don't print large responses, or repeated ones
                    continue;

                // XXX the strnlen() test is super-hacky
                if (do_h3 && n == 0 &&
                    (v->buf[0] != 0x01 && v->buf[0] != 0xff &&
                     strnlen((char *)v->buf, v->len) == v->len))
                    warn(WRN, "no h3 payload");
                if (n < 4 || is_last) {
                    if (do_h3) {
#ifndef NDEBUG
                        if (util_dlevel == DBG)
                            hexdump(v->buf, v->len);
#endif
                    } else {
                        // don't print newlines to console log
                        for (uint16_t p = 0; p < v->len; p++)
                            if (v->buf[p] == '\n' || v->buf[p] == '\r')
                                v->buf[p] = ' ';
                        printf("%.*s%s", v->len, v->buf, is_last ? "\n" : "");
                        if (is_last)
                            fflush(stdout);
                    }
                } else
                    printf(".");
                n++;
            }

            q_free_stream(se->s);
            free_sl_head();
        }
    }

    q_cleanup(w);
    free_cc(cc);
    free_sl();
    warn(DBG, "%s exiting", basename(argv[0]));
    return ret;
}
