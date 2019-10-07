/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "PMIxPlugin.h"

#include <fairmq/Tools.h>
#include <stdexcept>

namespace fair
{
namespace mq
{
namespace plugins
{

PMIxPlugin::PMIxPlugin(const std::string& name,
                       const Plugin::Version version,
                       const std::string& maintainer,
                       const std::string& homepage,
                       PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fPid(getpid())
{
    Init();
    SetProperty<std::string>("id", std::string(fProc.nspace) + "_" + std::to_string(fProc.rank));
    Fence();

    SubscribeToDeviceStateChange([&](DeviceState newState) {
        switch (newState) {
        case DeviceState::Idle:
            Fence();
            break;
        case DeviceState::Bound:
            Publish();
            Fence();
            break;
        case DeviceState::Connecting:
            Lookup();
            break;
        case DeviceState::DeviceReady:
            Fence();
            break;
        case DeviceState::Ready:
            Fence();
            break;
        case DeviceState::Exiting:
            UnsubscribeFromDeviceStateChange();
            break;
        default:
            break;
        }
    });
}

PMIxPlugin::~PMIxPlugin()
{
    while (pmix::initialized()) {
        try {
            pmix::finalize();
            LOG(debug) << PMIxClient() << "pmix::finalize() OK";
        } catch (const pmix::runtime_error& e) {
            LOG(debug) << PMIxClient() << "pmix::finalize() failed: " << e.what();
        }
    }
}

auto PMIxPlugin::PMIxClient() const -> std::string
{
    std::stringstream ss;
    ss << "PMIx client(pid=" << fPid << ") ";
    return ss.str();
}

auto PMIxPlugin::Init() -> void
{
    if (!pmix::initialized()) {
        fProc = pmix::init();
        LOG(debug) << PMIxClient() << "pmix::init() OK: " << fProc
                   << ",version=" << pmix::get_version();
    }
}

auto PMIxPlugin::Publish() -> void
{
    auto channels(GetChannelInfo());
    std::vector<pmix::info> info;

    for (const auto& c : channels) {
        std::string methodKey{"chans." + c.first + "." + std::to_string(c.second - 1) + ".method"};
        if (GetProperty<std::string>(methodKey) == "bind") {
            for (int i = 0; i < c.second; ++i) {
                std::string addressKey{"chans." + c.first + "." + std::to_string(i) + ".address"};
                info.emplace_back(addressKey, GetProperty<std::string>(addressKey));
                LOG(debug) << PMIxClient() << info.back();
            }
        }
    }

    if (info.size() > 0) {
        pmix::publish(info);
        LOG(debug) << PMIxClient() << "pmix::publish() OK: published "
                   << info.size() << " binding channels.";
    }
}

auto PMIxPlugin::Fence() -> void
{
    pmix::proc all(fProc);
    all.rank = pmix::rank::wildcard;

    pmix::fence({all});
    LOG(debug) << PMIxClient() << "pmix::fence() OK";
}

auto PMIxPlugin::Lookup() -> void
{
    auto channels(GetChannelInfo());
    for (const auto& c : channels) {
        std::string methodKey{"chans." + c.first + "." + std::to_string(c.second - 1) + ".method"};
        if (GetProperty<std::string>(methodKey) == "connect") {
            for (int i = 0; i < c.second; ++i) {
                std::vector<pmix::pdata> pdata;
                std::string addressKey{"chans." + c.first + "." + std::to_string(i) + ".address"};
                pdata.emplace_back();
                pdata.back().set_key(addressKey);
                std::vector<pmix::info> info;
                info.emplace_back(PMIX_WAIT, static_cast<int>(pdata.size()));

                if (pdata.size() > 0) {
                    pmix::lookup(pdata, info);
                    LOG(debug) << PMIxClient() << "pmix::lookup() OK";
                }

                for (const auto& p : pdata) {
                    if (p.value.type == PMIX_UNDEF) {
                        LOG(debug) << PMIxClient() << "pmix::lookup() not found: key=" << p.key;
                    } else if (p.value.type == PMIX_STRING) {
                        LOG(debug) << PMIxClient() << "pmix::lookup() found:"
				   << " key=" << p.key << ",value=" << p.value.data.string;
                        SetProperty<std::string>(p.key, p.value.data.string);
                    } else {
                        LOG(debug) << PMIxClient() << "pmix::lookup() wrong type returned: "
				   << "key=" << p.key << ",type=" << p.value.type;
                    }
                }
            }
        }
    }
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
