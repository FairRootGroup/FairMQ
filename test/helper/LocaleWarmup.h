/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_LOCALEWARMUP_H
#define FAIR_MQ_TEST_LOCALEWARMUP_H

#include <locale>
#include <sstream>

namespace fair::mq::test {

/// libstdc++'s std::ctype<char> caches narrow()/widen() results per character
/// in plain char arrays of the global classic-locale facet -- by design with
/// unsynchronized writes, emitted from header-inlined code (locale_facets.h).
/// Whenever two threads exercise an uncached character concurrently (e.g. by
/// compiling a std::regex), ThreadSanitizer rightfully reports the write as a
/// data race. The stores are real and unsynchronized, so instrumenting
/// libstdc++ cannot help; instead, fill the caches before any thread exists,
/// which turns every later access into a pure read.
inline void WarmUpLocaleCaches()
{
    auto const& ct = std::use_facet<std::ctype<char>>(std::locale::classic());
    for (int c = 1; c < 256; ++c) {
        auto const ch = static_cast<char>(c);
        ct.narrow(ch, '\0');
        ct.widen(ch);
    }

    // Only the range overload runs _M_narrow_init(), which sets the
    // all-cached flag (_M_narrow_ok); the single-char loop above fills the
    // cache without it. (widen() needs no equivalent: the single-char
    // overload already runs _M_widen_init().)
    char from[256];
    char to[256];
    for (int c = 0; c < 256; ++c) {
        from[c] = static_cast<char>(c);
    }
    ct.narrow(from, from + 256, '?', to);

    // num_put/num_get caches used by stream insertion/extraction
    std::ostringstream os;
    os << 4711 << ' ' << 3.14;
    std::istringstream is("42 3.14");
    int i = 0;
    double d = 0.;
    is >> i >> d;
}

/// For binaries that do not own main(): define a namespace-scope instance to
/// run the warm-up during static initialization, before main().
struct LocaleWarmup
{
    LocaleWarmup() { WarmUpLocaleCaches(); }
};

}   // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_LOCALEWARMUP_H */
