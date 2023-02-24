/********************************************************************************
 * Copyright (C) 2017-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Config.h"

#include <fairmq/JSONParser.h>
#include <fairmq/SuboptParser.h>

#include <vector>

using namespace std;

namespace fair::mq::plugins
{

Config::Config(const string& name, Plugin::Version version, const string& maintainer, const string& homepage, PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
{
    SubscribeToDeviceStateChange([&](DeviceState newState) {
        if (newState == DeviceState::InitializingDevice) {
            string idForParser;

            if (PropertyExists("config-key")) {
                idForParser = GetProperty<string>("config-key");
            } else if (PropertyExists("id")) {
                idForParser = GetProperty<string>("id");
            }

            if (!idForParser.empty()) {
                try {
                    if (PropertyExists("mq-config")) {
                        LOG(debug) << "mq-config: Using default JSON parser";
                        SetProperties(JSONParser(GetProperty<string>("mq-config"), idForParser));
                    } else if (PropertyExists("channel-config")) {
                        LOG(debug) << "channel-config: Parsing channel configuration";
                        SetProperties(SuboptParser(GetProperty<vector<string>>("channel-config"), idForParser));
                    } else {
                        LOG(info) << "fair::mq::plugins::Config: no channels configuration provided via --mq-config or --channel-config";
                    }
                } catch (exception& e) {
                    LOG(error) << e.what();
                }
            } else {
                LOG(warn) << "No device ID or config-key provided for the configuration parser.";
            }
        }
    });
}

Plugin::ProgOptions ConfigPluginProgramOptions()
{
    namespace po = boost::program_options;
    auto pluginOptions = po::options_description{"FairMQ device options"};
    pluginOptions.add_options()
        ("id",                            po::value<string        >()->default_value(""),                "Device ID.")
        ("io-threads",                    po::value<int           >()->default_value(1),                 "Number of I/O threads.")
        ("transport",                     po::value<string        >()->default_value("zeromq"),          "Transport ('zeromq'/'shmem').")
        ("network-interface",             po::value<string        >()->default_value("default"),         "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
        ("init-timeout",                  po::value<int           >()->default_value(120),               "Timeout for the initialization in seconds (when expecting dynamic initialization).")
        ("print-channels",                po::value<bool          >()->implicit_value(true),             "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
        ("shm-segment-size",              po::value<size_t        >()->default_value(2ULL << 30),        "Shared memory: size of the shared memory segment (in bytes).")
        ("shm-allocation",                po::value<string        >()->default_value("rbtree_best_fit"), "Shared memory allocation algorithm: rbtree_best_fit/simple_seq_fit.")
        ("shm-segment-id",                po::value<uint16_t      >()->default_value(0),                 "EXPERIMENTAL: Shared memory segment id for message creation.")
        ("shmid",                         po::value<uint64_t      >(),                                   "EXPERIMENTAL: Fixed shmid to use instead of deriving it from the session name.")
        ("shm-mlock-segment",             po::value<bool          >()->default_value(false),             "Shared memory: mlock the shared memory segment after initialization (opened or created).")
        ("shm-mlock-segment-on-creation", po::value<bool          >()->default_value(false),             "Shared memory: mlock the shared memory segment only once when created.")
        ("shm-zero-segment",              po::value<bool          >()->default_value(false),             "Shared memory: zero the shared memory segment memory after initialization (opened or created).")
        ("shm-zero-segment-on-creation",  po::value<bool          >()->default_value(false),             "Shared memory: zero the shared memory segment memory only once when created.")
        ("shm-throw-bad-alloc",           po::value<bool          >()->default_value(true),              "Shared memory: throw fair::mq::MessageBadAlloc if cannot allocate a message (retry if false).")
        ("bad-alloc-max-attempts",        po::value<int           >(),                                   "Maximum number of allocation attempts before throwing fair::mq::MessageBadAlloc. -1 is infinite. There is always at least one attempt, so 0 has safe effect as 1.")
        ("bad-alloc-attempt-interval",    po::value<int           >()->default_value(50),                "Interval between attempts if cannot allocate a message (in ms).")
        ("shm-monitor",                   po::value<bool          >()->default_value(false),             "Shared memory: run monitor daemon.")
        ("shm-no-cleanup",                po::value<bool          >()->default_value(false),             "Shared memory: do not cleanup the memory when last device leaves.")
        ("ofi-size-hint",                 po::value<size_t        >()->default_value(0),                 "EXPERIMENTAL: OFI size hint for the allocator.")
        ("rate",                          po::value<float         >()->default_value(0.),                "Rate for conditional run loop (Hz).")
        ("session",                       po::value<string        >()->default_value("default"),         "Session name.")
        ("config-key",                    po::value<string        >(),                                   "Use provided value instead of device id for fetching the configuration from JSON file.")
        ("mq-config",                     po::value<string        >(),                                   "JSON input as file.")
        ("channel-config",                po::value<vector<string>>()->multitoken()->composing(),        "Configuration of single or multiple channel(s) by comma separated key=value list");
    return pluginOptions;
}

Config::~Config()
{
    UnsubscribeFromDeviceStateChange();
}

} // namespace fair::mq::plugins
