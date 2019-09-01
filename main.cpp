#include <cstdlib>
#include <fairlogger/Logger.h>
#include <fairmq/SDK.h>
#include <fairmq/Version.h>
#include <string>

int main(int argc, char *argv[]) {
  fair::Logger::SetConsoleSeverity("debug");

  // workaround https://github.com/FairRootGroup/DDS/issues/24
  std::string path(std::getenv("PATH"));
  path = std::string("@FairMQ_BINDIR@:") + path;
  setenv("PATH", path.c_str(), 1);

  LOG(debug) << "FairMQ " << FAIRMQ_GIT_VERSION << " build "
             << FAIRMQ_BUILD_TYPE;

  fair::mq::sdk::DDSEnvironment ddsEnv;
  LOG(debug) << ddsEnv;

  fair::mq::sdk::DDSSession ddsSession(ddsEnv);
  ddsSession.StopOnDestruction();
  LOG(debug) << ddsSession;

  fair::mq::sdk::DDSTopology ddsTopo(
      "@FairMQ_DATADIR@/ex-dds-topology-infinite.xml", ddsEnv);
  LOG(debug) << ddsTopo;

  ddsSession.SubmitAgents(ddsTopo.GetNumRequiredAgents());

  ddsSession.ActivateTopology(ddsTopo);

  for (const auto &ddsAgent : ddsSession.RequestAgentInfo()) {
    LOG(debug) << ddsAgent;
  }

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
      // End transition does not yet wait for devices to acknowledge in v1.4.8
      if (transition != TopologyTransition::End) {
        LOG(info) << states.size() << " devices transitioned to "
                  << fair::mq::sdk::AggregateState(states);
      }
    } else {
      LOG(error) << ec;
      return ec.value();
    }
  }

  return 0;
}
