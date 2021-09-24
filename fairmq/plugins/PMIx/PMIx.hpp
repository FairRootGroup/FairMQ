/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef PMIX_HPP
#define PMIX_HPP

#include <array>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <ostream>
#include <pmix.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// C++ PMIx v2.2 API
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
        return os << p.nspace << "_" << p.rank;
    }
};

struct value : pmix_value_t
{
    value() { PMIX_VALUE_CONSTRUCT(static_cast<pmix_value_t*>(this)); }
    ~value() { PMIX_VALUE_DESTRUCT(static_cast<pmix_value_t*>(this)); }

    value(const value& rhs)
    {
        status rc;
        auto lhs(static_cast<pmix_value_t*>(this));
        PMIX_VALUE_XFER(rc, lhs, static_cast<pmix_value_t*>(const_cast<value*>(&rhs)));

        if (rc != PMIX_SUCCESS) {
            throw runtime_error("pmix::value copy ctor failed: rc=" + std::to_string(rc));
        }
    }

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

    explicit value(int val)
    {
        PMIX_VALUE_LOAD(static_cast<pmix_value_t*>(this), &val, PMIX_INT);
    }

    explicit value(pmix_data_array_t* val)
    {
        PMIX_VALUE_LOAD(static_cast<pmix_value_t*>(this), val, PMIX_DATA_ARRAY);
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
            throw runtime_error("pmix::info ctor failed: rc=" + std::to_string(rc));
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const info& i)
    {
        return os << "key=" << i.key << ",value='" << i.value.data.string << "'";
    }

    info(const info& rhs)
    {
        PMIX_INFO_XFER(static_cast<pmix_info_t*>(this),
                       static_cast<pmix_info_t*>(const_cast<info*>(&rhs)));
    }
};

struct pdata : pmix_pdata_t
{
    pdata() { PMIX_PDATA_CONSTRUCT(static_cast<pmix_pdata_t*>(this)); }
    ~pdata() { PMIX_PDATA_DESTRUCT(static_cast<pmix_pdata_t*>(this)); }

    pdata(const pdata& rhs)
    {
        PMIX_PDATA_XFER(static_cast<pmix_pdata_t*>(this),
                        static_cast<pmix_pdata_t*>(const_cast<pdata*>(&rhs)));
    }

    auto set_key(const std::string& new_key) -> void
    {
        (void)strncpy(key, new_key.c_str(), PMIX_MAX_KEYLEN);
    }
};

auto init(const std::vector<info>& info = {}) -> proc
{
    proc res;
    status rc;

    rc = PMIx_Init(&res, const_cast<pmix::info*>(info.data()), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::init() failed: rc=" + std::to_string(rc));
    }

    return res;
}

auto initialized() -> bool { return !!PMIx_Initialized(); }

auto get_version() -> std::string { return {PMIx_Get_version()}; }

auto finalize(const std::vector<info>& info = {}) -> void
{
    status rc;

    rc = PMIx_Finalize(info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::finalize() failed: rc=" + std::to_string(rc));
    }
}

auto publish(const std::vector<info>& info) -> void
{
    status rc;

    rc = PMIx_Publish(info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::publish() failed: rc=" + std::to_string(rc));
    }
}

auto fence(const std::vector<proc>& procs = {}, const std::vector<info>& info = {}) -> void
{
    status rc;

    rc = PMIx_Fence(procs.data(), procs.size(), info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::fence() failed: rc=" + std::to_string(rc));
    }
}

auto lookup(std::vector<pdata>& pdata, const std::vector<info>& info = {}) -> void
{
    status rc;

    rc = PMIx_Lookup(pdata.data(), pdata.size(), info.data(), info.size());
    if (rc != PMIX_SUCCESS) {
        throw runtime_error("pmix::lookup() failed: rc=" + std::to_string(rc));
    }
}

std::string get_info(const std::string& name, pmix::proc& process)
{
    pmix_value_t* v;

    pmix::status rc = PMIx_Get(&process, name.c_str(), nullptr, 0, &v);
    if (rc == PMIX_SUCCESS) {
        std::stringstream ss;

        switch (v->type) {
            case PMIX_SIZE:      ss << static_cast<size_t>(v->data.size)       << " (size_t)";       break;
            case PMIX_INT:       ss << static_cast<int>(v->data.integer)       << " (int)";          break;
            case PMIX_INT8:      ss << static_cast<int8_t>(v->data.int8)       << " (int8_t)";       break;
            case PMIX_INT16:     ss << static_cast<int16_t>(v->data.int16)     << " (int16_t)";      break;
            case PMIX_INT32:     ss << static_cast<int32_t>(v->data.int32)     << " (int32_t)";      break;
            case PMIX_INT64:     ss << static_cast<int64_t>(v->data.int64)     << " (int64_t)";      break;
            case PMIX_UINT:      ss << static_cast<unsigned int>(v->data.uint) << " (unsigned int)"; break;
            case PMIX_UINT8:     ss << static_cast<uint8_t>(v->data.uint8)     << " (uint8_t)";      break;
            case PMIX_UINT16:    ss << static_cast<uint16_t>(v->data.uint16)   << " (uint16_t)";     break;
            case PMIX_UINT32:    ss << static_cast<uint32_t>(v->data.uint32)   << " (uint32_t)";     break;
            case PMIX_UINT64:    ss << static_cast<uint64_t>(v->data.uint64)   << " (uint64_t)";     break;
            case PMIX_FLOAT:     ss << static_cast<float>(v->data.fval)        << " (float)";        break;
            case PMIX_DOUBLE:    ss << static_cast<double>(v->data.dval)       << " (double)";       break;
            case PMIX_PID:       ss << static_cast<pid_t>(v->data.pid)         << " (pid_t)";        break;
            case PMIX_STRING:    ss << static_cast<char*>(v->data.string)      << " (string)";       break;
            case PMIX_PROC_RANK: ss << static_cast<uint32_t>(v->data.rank)     << " (pmix_rank_t)";  break;
            case PMIX_PROC:      ss << "proc.nspace: " << static_cast<pmix_proc_t*>(v->data.proc)->nspace
                                    << ", proc.rank: " << static_cast<pmix_proc_t*>(v->data.proc)->rank << " (pmix_proc_t*)"; break;
            default:
                ss << "unknown type: " << v->type;
                break;
        }

        return ss.str();
    } else if (rc == PMIX_ERR_NOT_FOUND) {
        // LOG(error) << "PMIx_Get failed: PMIX_ERR_NOT_FOUND";
        return "";
    } else {
        // LOG(error) << "PMIx_Get failed: " << rc;
        return "<undefined>";
    }
}

std::string get_value_str(const pmix_value_t& v)
{
    switch (v.type) {
        case PMIX_BOOL:       return std::to_string(static_cast<bool>(v.data.flag));
        case PMIX_SIZE:       return std::to_string(static_cast<size_t>(v.data.size));
        case PMIX_INT:        return std::to_string(static_cast<int>(v.data.integer));
        case PMIX_INT8:       return std::to_string(static_cast<int8_t>(v.data.int8));
        case PMIX_INT16:      return std::to_string(static_cast<int16_t>(v.data.int16));
        case PMIX_INT32:      return std::to_string(static_cast<int32_t>(v.data.int32));
        case PMIX_INT64:      return std::to_string(static_cast<int64_t>(v.data.int64));
        case PMIX_UINT:       return std::to_string(static_cast<unsigned int>(v.data.uint));
        case PMIX_UINT8:      return std::to_string(static_cast<uint8_t>(v.data.uint8));
        case PMIX_UINT16:     return std::to_string(static_cast<uint16_t>(v.data.uint16));
        case PMIX_UINT32:     return std::to_string(static_cast<uint32_t>(v.data.uint32));
        case PMIX_UINT64:     return std::to_string(static_cast<uint64_t>(v.data.uint64));
        case PMIX_FLOAT:      return std::to_string(static_cast<float>(v.data.fval));
        case PMIX_DOUBLE:     return std::to_string(static_cast<double>(v.data.dval));
        case PMIX_PID:        return std::to_string(static_cast<pid_t>(v.data.pid));
        case PMIX_STRING:     return static_cast<char*>(v.data.string);
        case PMIX_PROC_RANK:  return std::to_string(static_cast<uint32_t>(v.data.rank));
        case PMIX_POINTER:    { std::stringstream ss; ss << static_cast<void*>(v.data.ptr); return ss.str(); }
        case PMIX_DATA_ARRAY: {
            if (v.data.darray->type == PMIX_PROC) {
                std::stringstream ss;
                ss << "[";
                for (size_t i = 0; i < v.data.darray->size; ++i) {
                    ss << static_cast<pmix_proc_t*>(static_cast<pmix_data_array_t*>(v.data.darray)->array)[0].nspace;
                    ss << "_";
                    ss << static_cast<pmix_proc_t*>(static_cast<pmix_data_array_t*>(v.data.darray)->array)[0].rank;

                    if (i < v.data.darray->size - 1) {
                        ss << ",";
                    }
                }
                ss << "]";
                return ss.str();
            } else {
                return "UNKNOWN TYPE IN DATA ARRAY";
            }
        }
        default:             return "UNKNOWN TYPE";
    }
}

} /* namespace pmix */

#endif /* PMIX_HPP */
