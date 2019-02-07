/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef PMIX_HPP
#define PMIX_HPP

#include <cstring>
#include <limits>
#include <memory>
#include <ostream>
#include <pmix.h>
#include <stdexcept>
#include <string.h>
#include <type_traits>
#include <utility>
#include <vector>
#include <FairMQLogger.h>

// C++ PMIx v2.1 API
namespace pmix
{

struct runtime_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

using status = pmix_status_t;

using nspace = pmix_nspace_t;

using key = pmix_key_t;

using data_type = pmix_data_type_t;

struct rank
{
    enum named : pmix_rank_t
    {
        undef = PMIX_RANK_UNDEF,
        wildcard = PMIX_RANK_WILDCARD,
        local_node = PMIX_RANK_LOCAL_NODE
    };

    explicit rank(pmix_rank_t r)
        : m_value(r)
    {}

    operator pmix_rank_t() { return m_value; }

  private:
    pmix_rank_t m_value;
};

struct proc : pmix_proc_t
{
    proc() { PMIX_PROC_CONSTRUCT(static_cast<pmix_proc_t*>(this)); }
    ~proc() { PMIX_PROC_DESTRUCT(static_cast<pmix_proc_t*>(this)); }

    proc(pmix::nspace ns, pmix::rank r)
    {
        PMIX_PROC_LOAD(static_cast<pmix_proc_t*>(this), ns, static_cast<pmix_rank_t>(r));
    }

    friend std::ostream& operator<<(std::ostream& os, const proc& p)
    {
        return os << "nspace=" << p.nspace << ",rank=" << p.rank;
    }
};

struct value : pmix_value_t
{
    value() { PMIX_VALUE_CONSTRUCT(static_cast<pmix_value_t*>(this)); }
    ~value() { PMIX_VALUE_DESTRUCT(static_cast<pmix_value_t*>(this)); }

    value(const value& rhs)
    {
      LOG(warn) << "copy ctor";
        status rc;
        auto lhs(static_cast<pmix_value_t*>(this));
        PMIX_VALUE_XFER(rc, lhs, static_cast<pmix_value_t*>(const_cast<value*>(&rhs)));

        if (rc != PMIX_SUCCESS) {
            throw runtime_error("pmix::value copy ctor failed: rc=" + rc);
        }
    }

    // template<typename T>
    // value(const T* val, data_type dt)
    // {
        // PMIX_VALUE_LOAD(static_cast<pmix_value_t*>(this), const_cast<void*>(val), dt);
    // }

    template<typename T>
    explicit value(T)
    {
        throw runtime_error("Given value type not supported or not yet implemented.");
    }

    explicit value(const char* val)
    {
        PMIX_VALUE_LOAD(static_cast<pmix_value_t*>(this), const_cast<char*>(val), PMIX_STRING);
    }

    explicit value(const std::string& val)
    {
        PMIX_VALUE_LOAD(
            static_cast<pmix_value_t*>(this), const_cast<char*>(val.c_str()), PMIX_STRING);
    }
};

struct info : pmix_info_t
{
    info() { PMIX_INFO_CONSTRUCT(static_cast<pmix_info_t*>(this)); }
    ~info() { PMIX_INFO_DESTRUCT(static_cast<pmix_info_t*>(this)); }

    template<typename... Args>
    info(const std::string& k, Args&&... args)
    {
        (void)strncpy(key, k.c_str(), PMIX_MAX_KEYLEN);
        flags = 0;

        pmix::value rhs(std::forward<Args>(args)...);
        auto lhs(&value);
        status rc;
        PMIX_VALUE_XFER(rc, lhs, static_cast<pmix_value_t*>(&rhs));

        if (rc != PMIX_SUCCESS) {
            throw runtime_error("pmix::info ctor failed: rc=" + rc);
        }
    }
};

auto init(const std::vector<info>& info = {}) -> proc
{
    proc res;
    status rc;

    rc = PMIx_Init(&res, const_cast<pmix::info*>(info.data()), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::init() failed: rc=" + rc);
    }

    return res;
}

auto initialized() -> bool { return !!PMIx_Initialized(); }

auto get_version() -> const char* { return PMIx_Get_version(); }

auto finalize(const std::vector<info>& info = {}) -> void
{
    status rc;

    rc = PMIx_Finalize(info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::finalize() failed: rc=" + rc);
    }
}

auto publish(const std::vector<info>& info) -> void
{
    status rc;

    rc = PMIx_Publish(info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::publish() failed: rc=" + rc);
    }
}

} /* namespace pmix */

#endif /* PMIX_HPP */
