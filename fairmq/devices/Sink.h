/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SINK_H
#define FAIR_MQ_SINK_H

#include <FairMQPoller.h>
#include <fairmq/Device.h>
#include <fairmq/tools/Strings.h>

#include <chrono>
#include <fairlogger/Logger.h>
#include <fstream>
#include <string>
#include <stdexcept>

namespace fair::mq
{

class Sink : public Device
{
  protected:
    bool fMultipart = false;
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
    uint64_t fMaxFileSize = 0;
    uint64_t fBytesWritten = 0;
    std::string fInChannelName;
    std::string fOutFilename;
    std::fstream fOutputFile;

    void InitTask() override
    {
        fMultipart     = fConfig->GetProperty<bool>("multipart");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
        fMaxFileSize   = fConfig->GetProperty<uint64_t>("max-file-size");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutFilename   = fConfig->GetProperty<std::string>("out-filename");

        fBytesWritten = 0;
    }

    void Run() override
    {
        // store the channel reference to avoid traversing the map on every loop iteration
        FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

        LOG(info) << "Starting sink and expecting to receive " << fMaxIterations << " messages.";
        auto tStart = std::chrono::high_resolution_clock::now();

        if (!fOutFilename.empty()) {
            LOG(debug) << "Incoming messages will be written to file: " << fOutFilename;
            if (fMaxFileSize != 0) {
                LOG(debug) << "File output will stop after " << fMaxFileSize << " bytes";
            } else {
                LOG(debug) << "ATTENTION: --max-file-size is 0 - output file will continue to grow until sink is stopped";
            }

            fOutputFile.open(fOutFilename, std::ios::out | std::ios::binary);
            if (!fOutputFile) {
                LOG(error) << "Could not open '" << fOutFilename;
                throw std::runtime_error(fair::mq::tools::ToString("Could not open '", fOutFilename));
            }
        }

        while (!NewStatePending()) {
            if (fMultipart) {
                FairMQParts parts;
                if (dataInChannel.Receive(parts) < 0) {
                    continue;
                }
                if (fOutputFile.is_open()) {
                    for (const auto& part : parts) {
                        WriteToFile(static_cast<const char*>(part->GetData()), part->GetSize());
                    }
                }
            } else {
                FairMQMessagePtr msg(dataInChannel.NewMessage());
                if (dataInChannel.Receive(msg) < 0) {
                    continue;
                }
                if (fOutputFile.is_open()) {
                    WriteToFile(static_cast<const char*>(msg->GetData()), msg->GetSize());
                }
            }

            if (fMaxFileSize > 0 && fBytesWritten >= fMaxFileSize) {
                LOG(info) << "Written " << fBytesWritten << " bytes, stopping...";
                break;
            }
            if (fMaxIterations > 0) {
                if (fNumIterations >= fMaxIterations) {
                    LOG(info) << "Configured maximum number of iterations reached.";
                    break;
                }
            }
            fNumIterations++;
        }

        if (fOutputFile.is_open()) {
            fOutputFile.flush();
            fOutputFile.close();
        }

        auto tEnd = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        LOG(info) << "Received " << fNumIterations << " messages in " << ms << "ms.";
        if (!fOutFilename.empty()) {
            auto sec = std::chrono::duration<double>(tEnd - tStart).count();
            LOG(info) << "Closed '" << fOutFilename << "' after writing " << fBytesWritten << " bytes."
                      << "(" << (fBytesWritten / (1000. * 1000.)) / sec << " MB/s)";
        }

        LOG(info) << "Leaving RUNNING state.";
    }

    void WriteToFile(const char* ptr, size_t size)
    {
        fOutputFile.write(ptr, size);
        if (fOutputFile.bad()) {
            LOG(error) << "failed writing to file";
            throw std::runtime_error("failed writing to file");
        }
        fBytesWritten += size;
    }
};

} // namespace fair::mq

#endif /* FAIR_MQ_SINK_H */
