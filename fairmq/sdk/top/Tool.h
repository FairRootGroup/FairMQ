/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOP_TOOL_H
#define FAIR_MQ_SDK_TOP_TOOL_H

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <exception>
#include <fairmq/Tools.h>
#include <fairmq/Version.h>
#include <fairmq/sdk/DDSAgent.h>
#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSSession.h>
#include <fairmq/sdk/DDSTask.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/sdk/Topology.h>
#include <imgui/imgui.h>
#include <imtui/imtui-impl-ncurses.h>
#include <imtui/imtui.h>
#include <map>
#include <memory>
#include <system_error>

namespace fair::mq::sdk::top {

using namespace std::chrono_literals;

struct Tool
{
    static constexpr auto DefaultFrameDrawInterval = 200ms;   //   5Hz
    static constexpr auto DefaultInputPollInterval = 10ms;    // 100Hz
    static constexpr auto DefaultDataPollInterval = 333ms;    //   3Hz

    Tool(DDSSession::Id session_id)
        : fScreen(Tool::MakeScreen())
        , fSignals(fIoCtx, SIGINT, SIGTERM, SIGSEGV)
        , fFrameDrawInterval(DefaultFrameDrawInterval)
        , fFrameDrawTimer(fIoCtx)
        , fInputPollInterval(DefaultInputPollInterval)
        , fInputPollTimer(fIoCtx)
        , fDdsSessionId(std::move(session_id))
        , fDataPollInterval(DefaultInputPollInterval)
        , fDataPollTimer(fIoCtx)
    {
        fSignals.async_wait([&](std::error_code const&, int) {
            fIoCtx.stop();
            Tool::ShutdownImTui();
        });
    }
    Tool(Tool const&) = delete;
    Tool& operator=(Tool const&) = delete;
    Tool(Tool&&) = delete;
    Tool& operator=(Tool&&) = delete;
    ~Tool() { Tool::ShutdownImTui(); }

    auto DrawFrame() -> void
    {
        fFrameDrawTimer.expires_from_now(fFrameDrawInterval);   // this will drift a bit (depending
                                                                // on outstanding work in fIoCtx)
        ImTui_ImplNcurses_NewFrame();
        ImTui_ImplText_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
        ImGui::Begin(
            tools::ToString("fairmq-top v", FAIRMQ_VERSION, "  session: ", fDdsSessionId, "  active topo: ", fDdsTopo->GetName()).c_str(),
            nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::TextUnformatted(tools::ToString(this).c_str());
        // auto tasks(fDdsTopo->GetTasks());
        // for (auto const& task : tasks) {
        // ImGui::TextUnformatted(tools::ToString(task.GetId(), task.GetCollectionId()).c_str());
        // }
        // auto const agents(fDdsSession->RequestAgentInfo());
        // for (auto const& agent : agents) {
        // ImGui::TextUnformatted(tools::ToString(agent).c_str());
        // }
        // for (auto const& dev : fFmqTopoState) {
            // ImGui::TextUnformatted(
                // tools::ToString(dev.state, " ", fDdsTasks.at(dev.taskId)).c_str());
        // }
        ImGui::End();

        ImGui::Render();
        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), &fScreen);
        ImTui_ImplNcurses_DrawScreen();
        fFrameDrawTimer.async_wait([&](std::error_code const& e) {
            if (!e) {
                this->DrawFrame();
            }
        });
    }

    auto PollInput() -> void
    {
        fInputPollTimer.expires_from_now(fInputPollInterval);   // this will drift a bit (depending
                                                                // on outstanding work in fIoCtx)

        if (ImGui::IsKeyPressed('q', false)) {
            asio::post([&]() { fIoCtx.stop(); });
        }

        fInputPollTimer.async_wait([&](std::error_code const& e) {
            if (!e) {
                this->PollInput();
            }
        });
    }

    auto PollData() -> void
    {
        fDataPollTimer.expires_from_now(fDataPollInterval);   // this will drift a bit (depending
                                                              // on outstanding work in fIoCtx)

        fFmqTopoState = fFmqTopo->GetCurrentState();

        fDataPollTimer.async_wait([&](std::error_code const& e) {
            if (!e) {
                this->PollData();
            }
        });
    }

    auto LoadSession(DDSTopology::Path const& topo) -> void
    {
        if (fDdsSessionId.empty()) {
            fDdsSession = std::make_unique<DDSSession>(getMostRecentRunningDDSSession(fDdsEnv));
            fDdsSessionId = fDdsSession->GetId();
        } else {
            fDdsSession = std::make_unique<DDSSession>(fDdsSessionId);
        }
        fDdsTopo = std::make_unique<DDSTopology>(topo, fDdsEnv);
        fFmqTopo = std::make_unique<Topology>(fIoCtx.get_executor(), *fDdsTopo, *fDdsSession);

        this->PollData();
    }

    auto Run(DDSTopology::Path const& topo) -> int
    {
        asio::post([&]() { this->PollInput(); });
        asio::post([&]() { this->LoadSession(topo); });
        asio::post([&]() { this->DrawFrame(); });
        try {
            fIoCtx.run();
        } catch (...) {
            Tool::ShutdownImTui();
            throw;
        }
        return 0;
    }

  private:
    asio::io_context fIoCtx;
    ImTui::TScreen& fScreen;
    asio::signal_set fSignals;
    std::chrono::milliseconds fFrameDrawInterval;
    asio::steady_timer fFrameDrawTimer;
    std::chrono::milliseconds fInputPollInterval;
    asio::steady_timer fInputPollTimer;
    DDSSession::Id fDdsSessionId;

    // TODO refactor ff members into separate class
    DDSEnvironment fDdsEnv;
    std::unique_ptr<DDSSession> fDdsSession;
    std::unique_ptr<DDSTopology> fDdsTopo;
    std::unique_ptr<Topology> fFmqTopo;
    std::chrono::milliseconds fDataPollInterval;
    asio::steady_timer fDataPollTimer;
    TopologyState fFmqTopoState;
    std::map<DDSTask::Id, DDSCollection::Id> fDdsTasks;

    static auto MakeScreen() -> ImTui::TScreen&
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& screen(*ImTui_ImplNcurses_Init(true));
        ImTui_ImplText_Init();
        return screen;
    }

    static auto ShutdownImTui() -> void
    {
        ImTui_ImplText_Shutdown();
        ImTui_ImplNcurses_Shutdown();
    }
};

}   // namespace fair::mq::sdk::top

#endif /* FAIR_MQ_SDK_TOP_TOOL_H */
