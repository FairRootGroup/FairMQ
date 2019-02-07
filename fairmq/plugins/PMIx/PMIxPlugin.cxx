/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "PMIxPlugin.h"

#include <boost/algorithm/string/join.hpp>
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
    SubscribeToDeviceStateChange([&](DeviceState newState) {
        switch (newState) {
            case DeviceState::InitializingDevice:
                if (!pmix::initialized()) {
                    fProc = pmix::init();
                    LOG(debug) << PMIxClient() << " pmix::init() OK: " << fProc
                               << ",version=" << pmix::get_version();
                }

                FillChannelContainers();

                PublishBoundChannels();

                {
                    pmix::proc all(fProc);
                    all.rank = pmix::rank::wildcard;

                    pmix::fence({all});
                }

                // lookup
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
    LOG(debug) << PMIxClient() << " Finalizing PMIx session... (On success, logs seen by the RTE will stop here.)";

    while (pmix::initialized()) {
        try {
            pmix::finalize();
            LOG(debug) << PMIxClient() << " pmix::finalize() OK";
        } catch (const pmix::runtime_error& e) {
            LOG(debug) << PMIxClient() << " pmix::finalize() failed: " << e.what();
        }
    }
}

auto PMIxPlugin::FillChannelContainers() -> void
{
    try {
        std::unordered_map<std::string, int> channelInfo(GetChannelInfo());

        // fill binding and connecting chans
        for (const auto& c : channelInfo) {
            std::string methodKey{"chans." + c.first + "." + std::to_string(c.second - 1)
                                  + ".method"};
            if (GetProperty<std::string>(methodKey) == "bind") {
                fBindingChannels.insert(std::make_pair(c.first, std::vector<std::string>()));
                for (int i = 0; i < c.second; ++i) {
                    fBindingChannels.at(c.first).push_back(GetProperty<std::string>(
                        std::string{"chans." + c.first + "." + std::to_string(i) + ".address"}));
                }
            } else if (GetProperty<std::string>(methodKey) == "connect") {
                fConnectingChannels.insert(std::make_pair(c.first, ConnectingChannel()));
                LOG(debug) << "preparing to connect: " << c.first << " with " << c.second
                           << " sub-channels.";
                for (int i = 0; i < c.second; ++i) {
                    fConnectingChannels.at(c.first).fSubChannelAddresses.push_back(std::string());
                }
            } else {
                LOG(error) << "Cannot update address configuration. Channel method (bind/connect) "
                              "not specified.";
                return;
            }
        }
    } catch (const std::exception& e) {
        LOG(error) << "Error filling channel containers: " << e.what();
    }
}

auto PMIxPlugin::PublishBoundChannels() -> void
{
    std::vector<pmix::info> infos;
    infos.reserve(fBindingChannels.size());

    for (const auto& channel : fBindingChannels) {
        std::string joined = boost::algorithm::join(channel.second, ",");
        infos.emplace_back(channel.first, joined);
    }

    pmix::publish(infos);
    LOG(debug) << PMIxClient() << " pmix::publish() OK: published "
               << fBindingChannels.size() << " binding channels.";
}

} /* namespace plugins */
} /* namespace mq */
} /* namespace fair */
