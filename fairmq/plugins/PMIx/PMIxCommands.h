/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef PMIXCOMMANDS_H
#define PMIXCOMMANDS_H

#include "PMIx.hpp"

#include <fairlogger/Logger.h>
#include <fairmq/tools/Semaphore.h>
#include <memory> // make_unique
#include <string>

namespace pmix
{

std::array<std::string, 47> typeNames =
{
    {
        "PMIX_UNDEF",
        "PMIX_BOOL",
        "PMIX_BYTE",
        "PMIX_STRING",
        "PMIX_SIZE",
        "PMIX_PID",
        "PMIX_INT",
        "PMIX_INT8",
        "PMIX_INT16",
        "PMIX_INT32",
        "PMIX_INT64",
        "PMIX_UINT",
        "PMIX_UINT8",
        "PMIX_UINT16",
        "PMIX_UINT32",
        "PMIX_UINT64",
        "PMIX_FLOAT",
        "PMIX_DOUBLE",
        "PMIX_TIMEVAL",
        "PMIX_TIME",
        "PMIX_STATUS",
        "PMIX_VALUE",
        "PMIX_PROC",
        "PMIX_APP",
        "PMIX_INFO",
        "PMIX_PDATA",
        "PMIX_BUFFER",
        "PMIX_BYTE_OBJECT",
        "PMIX_KVAL",
        "PMIX_MODEX",
        "PMIX_PERSIST",
        "PMIX_POINTER",
        "PMIX_SCOPE",
        "PMIX_DATA_RANGE",
        "PMIX_COMMAND",
        "PMIX_INFO_DIRECTIVES",
        "PMIX_DATA_TYPE",
        "PMIX_PROC_STATE",
        "PMIX_PROC_INFO",
        "PMIX_DATA_ARRAY",
        "PMIX_PROC_RANK",
        "PMIX_QUERY",
        "PMIX_COMPRESSED_STRING",
        "PMIX_ALLOC_DIRECTIVE",
        "PMIX_INFO_ARRAY",
        "PMIX_IOF_CHANNEL",
        "PMIX_ENVAR"
    }
};

enum class Command : int
{
    general = PMIX_EXTERNAL_ERR_BASE,
    error = PMIX_EXTERNAL_ERR_BASE - 1
};


class Commands
{
  public:
    Commands(const proc& process)
        : fProcess(process)
        , fSubscribed(false)
    {
    }

    ~Commands()
    {
        Unsubscribe();
    }

    void Subscribe(std::function<void(const std::string& msg, const proc& sender)> callback)
    {
        using namespace std::placeholders;

        LOG(debug) << "PMIxCommands: Subscribing...";

        fCallback = callback;
        std::array<pmix::status, 1> codes;
        codes[0] = static_cast<int>(pmix::Command::general);

        PMIX_INFO_LOAD(&(fInfos[0]), PMIX_EVENT_RETURN_OBJECT, this, PMIX_POINTER);

        PMIx_Register_event_handler(codes.data(), codes.size(),
                                    fInfos.data(), fInfos.size(),
                                    &Commands::Handler,
                                    &Commands::EventHandlerRegistration,
                                    this);
        fBlocker.Wait();
        LOG(debug) << "PMIxCommands: Subscribing complete!";
    }

    void Unsubscribe()
    {
        if (fSubscribed) {
            LOG(debug) << "PMIxCommands: Unsubscribing...";
            PMIx_Deregister_event_handler(fHandlerRef, &Commands::EventHandlerDeregistration, this);
            fBlocker.Wait();
            LOG(debug) << "PMIxCommands: Unsubscribing complete!";
        } else {
            LOG(debug) << "Unsubscribe() is called while no subscription is active";
        }
    }

    struct Holder
    {
        Holder() : fData(nullptr) {}
        ~Holder() { PMIX_DATA_ARRAY_FREE(fData); }

        std::vector<pmix::info> fInfos;
        pmix_data_array_t* fData;
    };

    void Send(const std::string& msg)
    {
        std::vector<pmix::info>* infos = new std::vector<pmix::info>();
        infos->emplace_back("fairmq.cmd", msg);
        PMIx_Notify_event(static_cast<int>(pmix::Command::general),
                          &fProcess,
                          PMIX_RANGE_NAMESPACE,
                          infos->data(), infos->size(),
                          &Commands::OpCompleteCallback<std::vector<pmix::info>>,
                          infos);
    }

    void Send(const std::string& msg, rank rank)
    {
        pmix::proc destination(fProcess);
        destination.rank = rank;
        Send(msg, {destination});
    }

    void Send(const std::string& msg, const std::vector<proc>& destination)
    {
        std::unique_ptr<Holder> holder = std::make_unique<Holder>();

        PMIX_DATA_ARRAY_CREATE(holder->fData, destination.size(), PMIX_PROC);
        memcpy(holder->fData->array, destination.data(), destination.size() * sizeof(pmix_proc_t));
        // LOG(warn) << "OLOG: " << msg << " > " << static_cast<pmix_proc_t*>(holder->fData->array)[0].nspace << ": " << static_cast<pmix_proc_t*>(holder->fData->array)[0].rank;
        holder->fInfos.emplace_back(PMIX_EVENT_CUSTOM_RANGE, holder->fData);
        // LOG(warn) << msg << " // packed range: " << static_cast<pmix_proc_t*>(static_cast<pmix_data_array_t*>(holder->fInfos.at(0).value.data.darray)->array)[0].nspace << "_" << static_cast<pmix_proc_t*>(static_cast<pmix_data_array_t*>(holder->fInfos.at(0).value.data.darray)->array)[0].rank;
        // LOG(warn) << msg << " // packed range.type: " << pmix::typeNames.at(holder->fInfos.at(0).value.type);
        // LOG(warn) << msg << " // packed range.array.type: " << pmix::typeNames.at(static_cast<pmix_data_array_t*>(holder->fInfos.at(0).value.data.darray)->type);
        // LOG(warn) << msg << " // packed range.array.size: " << static_cast<pmix_data_array_t*>(holder->fInfos.at(0).value.data.darray)->size;
        // LOG(warn) << holder->fInfos.size();
        holder->fInfos.emplace_back("fairmq.cmd", msg);
        // LOG(warn) << msg << " // packed msg: " << holder->fInfos.at(1).value.data.string;
        // LOG(warn) << msg << " // packed msg.type: " << pmix::typeNames.at(holder->fInfos.at(1).value.type);
        // LOG(warn) << holder->fInfos.size();

        PMIx_Notify_event(static_cast<int>(pmix::Command::general),
                          &fProcess,
                          PMIX_RANGE_CUSTOM,
                          holder->fInfos.data(), holder->fInfos.size(),
                          &Commands::OpCompleteCallback<Holder>,
                          holder.get());
        holder.release();
    }

  private:
    static void EventHandlerRegistration(pmix_status_t s, size_t handlerRef, void* obj)
    {
        if (s == PMIX_SUCCESS) {
            LOG(debug) << "Successfully registered event handler, reference = " << static_cast<unsigned long>(handlerRef);
            static_cast<Commands*>(obj)->fHandlerRef = handlerRef;
            static_cast<Commands*>(obj)->fSubscribed = true;
        } else {
            LOG(error) << "Could not register PMIx event handler, status = " << s;
        }
        static_cast<Commands*>(obj)->fBlocker.Signal();
    }

    static void EventHandlerDeregistration(pmix_status_t s, void* obj)
    {
        if (s == PMIX_SUCCESS) {
            LOG(debug) << "Successfully deregistered event handler, reference = " << static_cast<Commands*>(obj)->fHandlerRef;
            static_cast<Commands*>(obj)->fSubscribed = false;
        } else {
            LOG(error) << "Could not deregister PMIx event handler, reference = " << static_cast<Commands*>(obj)->fHandlerRef << ", status = " << s;
        }
        static_cast<Commands*>(obj)->fBlocker.Signal();
    }

    template<typename T>
    static void OpCompleteCallback(pmix_status_t s, void* data)
    {
        if (s == PMIX_SUCCESS) {
            // LOG(info) << "Operation completed successfully";
        } else {
            LOG(error) << "Could not complete operation, status = " << s;
        }
        if (data) {
            // LOG(warn) << "Destroying event data...";
            delete static_cast<T*>(data);
        }
    }

    static void Handler(size_t handlerId,
                        pmix_status_t s,
                        const pmix_proc_t* src,
                        pmix_info_t info[], size_t ninfo,
                        pmix_info_t[] /* results */, size_t nresults,
                        pmix_event_notification_cbfunc_fn_t cbfunc,
                        void* cbdata)
    {
        std::stringstream ss;
        ss << "Event handler called with "
           << "status: "    << s                               << ", "
           << "source: "    << src->nspace << "_" << src->rank << ", "
           << "ninfo: "     << ninfo                           << ", "
           << "nresults: "  << nresults                        << ", "
           << "handlerId: " << handlerId;

        std::string msg;

        Commands* obj = nullptr;

        if (ninfo > 0) {
            ss << ":\n";
            for (size_t i = 0; i < ninfo; ++i) {
                ss << "   [" << i << "]: key: '"        << info[i].key
                                  << "', value: '"      << pmix::get_value_str(info[i].value)
                                  << "', value.type: '" << pmix::typeNames.at(info[i].value.type)
                                  << "', flags: "       << info[i].flags;

                if (std::strcmp(info[i].key, "fairmq.cmd") == 0) {
                    msg = pmix::get_value_str(info[i].value);
                }

                if (std::strcmp(info[i].key, PMIX_EVENT_RETURN_OBJECT) == 0) {
                    obj = static_cast<Commands*>(info[i].value.data.ptr);
                }

                if (i < ninfo - 1) {
                    ss << "\n";
                }
            }
        }


        if (obj != nullptr) {
            if (static_cast<Commands*>(obj)->fProcess.rank != src->rank) {
                // LOG(warn) << ss.str();
                static_cast<Commands*>(obj)->fCallback(msg, proc(const_cast<char*>(src->nspace), rank(src->rank)));
            } else {
                // LOG(trace) << "suppressing message from itself";
            }
        } else {
            LOG(error) << "ERROR";
        }

        if (cbfunc != nullptr) {
            cbfunc(PMIX_SUCCESS, nullptr, 0, nullptr, nullptr, cbdata);
        }
    }

    const proc& fProcess;
    size_t fHandlerRef;
    std::function<void(const std::string& msg, const proc& sender)> fCallback;
    std::array<pmix_info_t, 1> fInfos;
    bool fSubscribed;
    fair::mq::tools::SharedSemaphore fBlocker;
};

} /* namespace pmix */

#endif /* PMIXCOMMANDS_H */
