// Copyright (c) 2014-2018, NetApp, Inc.
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

#include <arpa/inet.h>
#include <net/if.h>

#include <benchmark/benchmark.h>
#include <quant/quant.h>
#include <warpcore/warpcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <picotls/openssl.h> // IWYU pragma: keep

#include "conn.h" // IWYU pragma: keep
#include "pkt.h"
#include "pn.h" // IWYU pragma: keep
#include "quic.h"
#include "tls.h" // IWYU pragma: keep

#ifdef __cplusplus
}
#endif


static struct q_conn * c;
static struct w_engine * w;


static void BM_quic_encryption(benchmark::State & state)
{
    const auto len = uint16_t(state.range(0));
    const auto pne = uint16_t(state.range(1));

    struct pkt_meta * m;
    struct w_iov * v = alloc_iov(w, len, 0, &m);
    struct pkt_meta * mx;
    struct w_iov * x = alloc_iov(w, MAX_PKT_LEN, 0, &mx);

    rand_bytes(v->buf, len);
    m->hdr.type = LH_INIT;
    m->hdr.flags = LH | m->hdr.type;
    m->hdr.hdr_len = 16;
    m->hdr.len = len;
    m->pn = pn_for_epoch(c, ep_init);

    for (auto _ : state)
        benchmark::DoNotOptimize(enc_aead(v, m, x, pne * 16));
    state.SetBytesProcessed(int64_t(state.iterations() * len)); // NOLINT

    free_iov(x, mx);
    free_iov(v, m);
}


BENCHMARK(BM_quic_encryption)
    ->RangeMultiplier(2)
    ->Ranges({{16, MAX_PKT_LEN}, {0, 1}})
    // ->MinTime(3)
    // ->UseRealTime()
    ;


// BENCHMARK_MAIN()

int main(int argc, char ** argv)
{
    char i[IFNAMSIZ] = "lo" // NOLINT
#ifndef __linux__
                       "0"
#endif
        ;

    benchmark::Initialize(&argc, argv);
#ifndef NDEBUG
    util_dlevel = INF;
#endif
    w = q_init(i, nullptr);
    struct cid cid = {};
    cid.len = 4;
    memcpy(cid.id, "1234", cid.len);
    c = new_conn(w, 0xff00000e, &cid, &cid, nullptr, "", bswap16(55555),
                 nullptr);
    init_tls(c, nullptr);
    benchmark::RunSpecifiedBenchmarks();

    q_cleanup(w);
}
