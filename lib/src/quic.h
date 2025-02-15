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

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

// IWYU pragma: no_include <picotls/../picotls.h>

#include <picotls.h> // IWYU pragma: keep

#include <quant/quant.h>

#include "frame.h"


// #define DEBUG_BUFFERS ///< Set to log buffer use details.
// #define DEBUG_EXTRA   ///< Set to log various extra details.
// #define DEBUG_STREAMS ///< Set to log stream scheduling details.
// #define DEBUG_TIMERS  ///< Set to log timer details.
// #define DEBUG_PROT    ///< Set to log packet protection/encryption details.

#define DATA_OFFSET 48 ///< Offsets of stream frame payload data we TX.

#define CID_LEN_MIN 4   ///< Minimum CID length allowed by spec.
#define CID_LEN_MAX 20  ///< Maximum CID length allowed by spec.
#define SCID_LEN_CLNT 4 ///< Default client source CID length.
#define SCID_LEN_SERV 8 ///< Default server source CID length.
#define SRT_LEN 16      ///< Stateless reset token length allowed by spec.
#define PATH_CHLG_LEN 8 ///< Length of a path challenge.

#ifdef PARTICLE
#define IPTOS_ECN_NOTECT 0x00 // not-ECT
#define IPTOS_ECN_ECT1 0x01   // ECN-capable transport (1)
#define IPTOS_ECN_ECT0 0x02   // ECN-capable transport (0)
#define IPTOS_ECN_CE 0x03     // congestion experienced
#define IPTOS_ECN_MASK 0x03   // ECN field mask

#define NI_MAXHOST 16
#define NI_MAXSERV 6
#define O_CLOEXEC 0
#endif

// Maximum reordering in packets before packet threshold loss detection
// considers a packet lost. The RECOMMENDED value is 3.
#define kPacketThreshold 3

// Maximum reordering in time before time threshold loss detection considers a
// packet lost. Specified as an RTT multiplier. The RECOMMENDED value is 9/8.
// #define kTimeThreshold 1.125

// Timer granularity. This is a system-dependent value. However, implementations
// SHOULD use a value no smaller than 1ms.
#define kGranularity 1 * NS_PER_MS

// The RTT used before an RTT sample is taken. The RECOMMENDED value is 100ms.
#define kInitialRtt 500 * NS_PER_MS

/// The sender's maximum payload size. Does not include UDP or IP overhead. The
/// max packet size is used for calculating initial and minimum congestion
/// windows.
#define kMaxDatagramSize 1200

/// Default limit on the initial amount of outstanding data in flight, in bytes.
/// Taken from [RFC6928]. The RECOMMENDED value is the minimum of 10 *
/// kMaxDatagramSize and max(2* kMaxDatagramSize, 14720)).
#define kInitialWindow                                                         \
    MIN(10 * kMaxDatagramSize, MAX(2 * kMaxDatagramSize, 14720))

/// Minimum congestion window in bytes.
#define kMinimumWindow (2 * kMaxDatagramSize)

/// Reduction in congestion window when a new loss event is detected.
#define kLossReductionDivisor 2 // kLossReductionFactor

/// Number of consecutive PTOs after which network is considered to be
/// experiencing persistent congestion. The RECOMMENDED value for
/// kPersistentCongestionThreshold is 3, which is equivalent to having two TLPs
/// before an RTO in TCP.
#define kPersistentCongestionThreshold 3


#ifndef PARTICLE
#define NRM "\x1B[0m" ///< ANSI escape sequence: reset all to normal
#define BLD "\x1B[1m" ///< ANSI escape sequence: bold
// #define DIM "\x1B[2m"   ///< ANSI escape sequence: dim
// #define ULN "\x1B[3m"   ///< ANSI escape sequence: underline
// #define BLN "\x1B[5m"   ///< ANSI escape sequence: blink
#define REV "\x1B[7m" ///< ANSI escape sequence: reverse
// #define HID "\x1B[8m"   ///< ANSI escape sequence: hidden
// #define BLK "\x1B[30m"  ///< ANSI escape sequence: black
#define RED "\x1B[31m" ///< ANSI escape sequence: red
#define GRN "\x1B[32m" ///< ANSI escape sequence: green
#define YEL "\x1B[33m" ///< ANSI escape sequence: yellow
#define BLU "\x1B[34m" ///< ANSI escape sequence: blue
#define MAG "\x1B[35m" ///< ANSI escape sequence: magenta
#define CYN "\x1B[36m" ///< ANSI escape sequence: cyan
// #define WHT "\x1B[37m"  ///< ANSI escape sequence: white
#else
#define NRM ""
#define BLD ""
#define REV ""
#define RED ""
#define GRN ""
#define YEL ""
#define BLU ""
#define MAG ""
#define CYN ""
#endif

#define FMT_PNR_IN BLU "%" PRIu NRM
#define FMT_PNR_OUT GRN "%" PRIu NRM
#define FMT_SID BLD YEL "%" PRId NRM


struct cid {
    splay_entry(cid) node_seq;
    uint_t seq; ///< Connection ID sequence number
    uint_t rpt; ///< Retire prior to
    /// XXX len must precede id for cid_cmp() over both to work
    uint8_t len; ///< Connection ID length
    /// XXX id must precede srt for rand_bytes() over both to work
    uint8_t id[CID_LEN_MAX]; ///< Connection ID
    uint8_t srt[SRT_LEN];    ///< Stateless Reset Token
    uint8_t retired : 1;     ///< Did we retire this CID?
    uint8_t has_srt : 1;     ///< Is the SRT field valid?
    uint8_t : 6;
    uint8_t _unused[2];
};


struct pkt_hdr {
    struct cid dcid;  ///< Destination CID.
    struct cid scid;  ///< Source CID.
    uint_t nr;        ///< Packet number.
    uint16_t len;     ///< Content of length field in long header.
    uint16_t hdr_len; ///< Length of entire QUIC header.
    uint32_t vers;    ///< QUIC version in long header.
    uint8_t flags;    ///< First (raw) byte of packet.
    uint8_t type;     ///< Parsed packet type.
    // we do not store any token of LH packets in the metadata anymore

#ifdef HAVE_64BIT
    uint8_t _unused[6];
#else
    uint8_t _unused[2];
#endif
};


/// Packet meta-data information associated with w_iov buffers
struct pkt_meta {
    // XXX need to potentially change pm_cpy() below if fields are reordered
    splay_entry(pkt_meta) off_node;
    sl_entry(pkt_meta) rtx_next;
    sl_head(pm_sl, pkt_meta) rtx; ///< List of pkt_meta structs of previous TXs.

    // pm_cpy(true) starts copying from here:
    struct frames frms;     ///< Frames present in pkt.
    struct q_stream * strm; ///< Stream this data was written on.
    uint_t strm_off;        ///< Stream data offset.
    uint16_t strm_frm_pos;  ///< Offset of stream frame header.
    uint16_t strm_data_pos; ///< Offset of first byte of stream frame data.
    uint16_t strm_data_len; ///< Length of stream frame data.

    uint16_t ack_frm_pos; ///< Offset of (first, on RX) ACK frame (+1 for type).

    dint_t max_strm_data_sid; ///< MAX_STREAM_DATA sid, if sent.
    uint_t max_strm_data;     ///< MAX_STREAM_DATA limit, if sent.
    uint_t max_data;          ///< MAX_DATA limit, if sent.
    dint_t max_strms_bidi;    ///< MAX_STREAM_ID bidir limit, if sent.
    dint_t max_strms_uni;     ///< MAX_STREAM_ID unidir limit, if sent.
    uint_t strm_data_blocked; ///< STREAM_DATA_BLOCKED value, if sent.
    uint_t data_blocked;      ///< DATA_BLOCKED value, if sent.
    uint_t min_cid_seq; ///< Smallest NEW_CONNECTION_ID seq in pkt, if sent.

#ifndef HAVE_64BIT
    uint8_t _unused[4];
#endif

    // pm_cpy(false) starts copying from here:
    struct pn_space * pn; ///< Packet number space.
    struct pkt_hdr hdr;   ///< Parsed packet header.
    uint64_t t;           ///< TX or RX timestamp.

    uint16_t udp_len;          ///< Length of protected UDP packet at TX/RX.
    uint8_t has_rtx : 1;       ///< Does the w_iov hold truncated data?
    uint8_t is_reset : 1;      ///< This packet is a stateless reset.
    uint8_t is_fin : 1;        ///< This packet has a stream FIN bit.
    uint8_t in_flight : 1;     ///< Does this pkt count towards in_flight?
    uint8_t ack_eliciting : 1; ///< Is this packet ACK-eliciting?

    uint8_t acked : 1; ///< Was this packet ACKed?
    uint8_t lost : 1;  ///< Have we marked this packet as lost?
    uint8_t txed : 1;  ///< Did we TX this pkt?

#ifdef HAVE_64BIT
    uint8_t _unused2[5];
#else
    uint8_t _unused2[1];
#endif
};


struct per_engine_data {
    struct timeouts * wheel;
    struct pkt_meta * pkt_meta;
    ptls_context_t tls_ctx;
};


#define ped(w) ((struct per_engine_data *)((w)->data))


extern struct q_conn_sl accept_queue;
extern struct q_conn_conf default_conn_conf;

/// The versions of QUIC supported by this implementation
extern const uint32_t ok_vers[];
extern const uint8_t ok_vers_len;


extern void __attribute__((nonnull)) alloc_off(struct w_engine * const w,
                                               struct w_iov_sq * const q,
                                               const uint32_t len,
                                               const uint16_t off);

extern void __attribute__((nonnull))
free_iov(struct w_iov * const v, struct pkt_meta * const m);


extern struct w_iov * __attribute__((nonnull))
alloc_iov(struct w_engine * const w,
          const uint16_t len,
          const uint16_t off,
          struct pkt_meta ** const m);


extern struct w_iov * __attribute__((nonnull(1)))
w_iov_dup(const struct w_iov * const v,
          struct pkt_meta ** const mdup,
          const uint16_t off);


#if (!defined(NDEBUG) || defined(NDEBUG_WITH_DLOG)) && !defined(FUZZING) &&    \
    !defined(NO_FUZZER_CORPUS_COLLECTION)
extern int corpus_pkt_dir, corpus_frm_dir;

extern void __attribute__((nonnull))
write_to_corpus(const int dir, const void * const data, const size_t len);
#endif


/// Is flag @p f set in flags variable @p v?
///
/// @param      f     Flag.
/// @param      v     Variable.
///
/// @return     True if set, false otherwise.
///
#define is_set(f, v) (((v) & (f)) == (f))


/// Return the pkt_meta entry for a given w_iov.
///
/// @param      v     Pointer to a w_iov.
///
/// @return     Pointer to the pkt_meta entry for the w_iov.
///
#define meta(v) ped((v)->w)->pkt_meta[w_iov_idx(v)]


/// Return the w_iov index of a given pkt_meta.
///
/// @param      w     Pointer to warpcore engine.
/// @param      m     Pointer to a pkt_meta entry.
///
/// @return     Index of the struct w_iov the struct pkt_meta holds meta data
///             for.
///
#define pm_idx(w, m) (uint32_t)((m)-ped(w)->pkt_meta)


extern char * __attribute__((nonnull)) hex2str(const uint8_t * const src,
                                               const size_t len_src,
                                               char * const dst,
                                               const size_t len_dst);

#define hex_str_len(x) ((x)*2 + 1)

#define mk_cid_str(lvl, cid, str)                                              \
    char str[DLEVEL >= (lvl)                                                   \
                 ? hex_str_len(2 * sizeof((cid)->seq) + CID_LEN_MAX)           \
                 : 1];                                                         \
    if (unlikely(DLEVEL >= (lvl)) && likely(cid)) {                            \
        const int _n = snprintf(str, sizeof(str), "%" PRIu ":", (cid)->seq);   \
        hex2str((cid)->id, (cid)->len, &str[_n], sizeof(str) - (size_t)_n);    \
    }

#define mk_path_chlg_str(lvl, path_chlg, str)                                  \
    char str[DLEVEL >= (lvl) ? hex_str_len(PATH_CHLG_LEN) : 1];                \
    if (unlikely(DLEVEL >= (lvl)))                                             \
    hex2str((path_chlg), sizeof(path_chlg), str, sizeof(str))


#define mk_srt_str(lvl, srt, str)                                              \
    char str[DLEVEL >= (lvl) ? hex_str_len(SRT_LEN) : 1];                      \
    if (unlikely(DLEVEL >= (lvl)))                                             \
    hex2str((srt), sizeof(srt), str, sizeof(str))


#define mk_tok_str(lvl, tok, tok_len, str)                                     \
    char str[DLEVEL >= (lvl) ? hex_str_len(MAX_TOK_LEN) : 1];                  \
    if (unlikely(DLEVEL >= (lvl)) && likely(tok)) {                            \
        hex2str((tok), (tok_len), str, sizeof(str));                           \
    }


#define has_strm_data(p) (p)->strm_frm_pos


#define get_conf(conf, val)                                                    \
    (conf) && (conf)->val ? (conf)->val : default_conn_conf.val


#define get_conf_uncond(conf, val) (conf) ? (conf)->val : default_conn_conf.val


static inline void __attribute__((nonnull))
cid_cpy(struct cid * const dst, const struct cid * const src)
{
    memcpy((uint8_t *)dst + offsetof(struct cid, seq),
           (const uint8_t *)src + offsetof(struct cid, seq),
           sizeof(struct cid) - offsetof(struct cid, seq));
}


static inline void __attribute__((nonnull))
pm_cpy(struct pkt_meta * const dst,
       const struct pkt_meta * const src,
       const bool also_frame_info)
{
    const size_t off = also_frame_info ? offsetof(struct pkt_meta, frms)
                                       : offsetof(struct pkt_meta, pn);
    memcpy((uint8_t *)dst + off, (const uint8_t *)src + off,
           sizeof(*dst) - off);
}


static inline void __attribute__((nonnull))
adj_iov_to_start(struct w_iov * const v, const struct pkt_meta * const m)
{
    v->buf -= m->strm_data_pos;
    v->len += m->strm_data_pos;
}


static inline void __attribute__((nonnull))
adj_iov_to_data(struct w_iov * const v, const struct pkt_meta * const m)
{
    v->buf += m->strm_data_pos;
    v->len -= m->strm_data_pos;
}
