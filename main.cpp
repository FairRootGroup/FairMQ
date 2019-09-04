#include <cstdlib>
#include <fairlogger/Logger.h>
#include <fairmq/SDK.h>
#include <fairmq/Version.h>
#include <string>

int main(int argc, char *argv[]) {
  fair::Logger::SetConsoleSeverity("info");
  fair::Logger::DefineVerbosity(
      "user1", fair::VerbositySpec::Make(fair::VerbositySpec::Info::timestamp_us,
                                         fair::VerbositySpec::Info::severity));
  fair::Logger::SetVerbosity("user1");
  fair::Logger::SetConsoleColor();

  // workaround https://github.com/FairRootGroup/DDS/issues/24
  std::string path(std::getenv("PATH"));
  path = std::string("@FairMQ_BINDIR@:") + path;
  setenv("PATH", path.c_str(), 1);

  LOG(info) << "FairMQ " << FAIRMQ_GIT_VERSION << " build "
            << FAIRMQ_BUILD_TYPE;

  fair::mq::sdk::DDSEnvironment ddsEnv;
  LOG(info) << ddsEnv;

  fair::mq::sdk::DDSSession ddsSession(ddsEnv);
  ddsSession.StopOnDestruction();
  LOG(info) << ddsSession;

  fair::mq::sdk::DDSTopology ddsTopo(
      "@FairMQ_DATADIR@/ex-dds-topology-infinite.xml", ddsEnv);
  LOG(info) << ddsTopo;

  ddsSession.SubmitAgents(ddsTopo.GetNumRequiredAgents());

  ddsSession.ActivateTopology(ddsTopo);
  auto ddsAgents(ddsSession.RequestAgentInfo());

  fair::mq::sdk::Topology fairmqTopo(ddsTopo, ddsSession);

  using fair::mq::sdk::TopologyTransition;
  for (auto transition :
       {TopologyTransition::InitDevice, TopologyTransition::CompleteInit,
        TopologyTransition::Bind, TopologyTransition::Connect,
        TopologyTransition::InitTask, TopologyTransition::Run,
        TopologyTransition::Stop, TopologyTransition::ResetTask,
        TopologyTransition::ResetDevice, TopologyTransition::End}) {
    auto [ec, states] =
        fairmqTopo.ChangeState(transition, std::chrono::milliseconds(300));
    if (!ec) {
      LOG(info) << states.size() << " devices transitioned to "
                << fair::mq::sdk::AggregateState(states);
    } else {
      LOG(error) << ec;
      return ec.value();
    }
  }

  LOG(info) << "DDS commander logs in " << ddsEnv.GetConfigHome() / ".DDS/log/sessions" / ddsSession.GetId();
  LOG(info) << "DDS agent logs in ";
  for (const auto& ddsAgent : ddsAgents) {
    LOG(info) << "  " << ddsAgent.GetDDSPath();
  }

  return 0;
}
