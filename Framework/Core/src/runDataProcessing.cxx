// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "FairMQDevice.h"
#include "Framework/ChannelMatching.h"
#include "Framework/DataProcessingDevice.h"
#include "Framework/DataProcessorSpec.h"
#include "Framework/DataSourceDevice.h"
#include "Framework/ConfigParamsHelper.h"
#include "Framework/DebugGUI.h"
#include "Framework/DeviceControl.h"
#include "Framework/DeviceInfo.h"
#include "Framework/DeviceSpec.h"
#include "Framework/DeviceMetricsInfo.h"
#include "Framework/FrameworkGUIDebugger.h"
#include "Framework/SimpleMetricsService.h"
#include "Framework/WorkflowSpec.h"
#include "Framework/LocalRootFileService.h"
#include "Framework/TextControlService.h"

#include "DDSConfigHelpers.h"
#include "options/FairMQProgOptions.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <getopt.h>
#include <csignal>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fairmq/DeviceRunner.h>

#include <boost/process.hpp>
namespace bp = boost::process;
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;
#include <boost/asio.hpp>
namespace basio = boost::asio;
#include <thread>
#include <functional>
#include <regex>

#include <dds_intercom.h>

using namespace o2::framework;

std::vector<DeviceInfo> gDeviceInfos;
std::vector<DeviceMetricsInfo> gDeviceMetricsInfos;
std::vector<DeviceControl> gDeviceControls;

// This is the handler for the parent inner loop.
// So far the only responsibility for it are:
//
// - Echo children output in a sensible manner
//
//
// - TODO: allow single child view?
// - TODO: allow last line per child mode?
// - TODO: allow last error per child mode?
int doGUI(std::vector<DeviceInfo>& infos,
          std::vector<DeviceSpec> specs,
          std::vector<DeviceControl> controls,
          std::vector<DeviceMetricsInfo> metricsInfos,
          basio::io_service& ios,
          bp::environment& ddsEnv,
          dds::intercom_api::CCustomCmd& ddsCustomCmd) {
  void *window = initGUI("O2 Framework debug GUI");
  // FIXME: I should really have some way of exiting the
  // parent..
  auto debugGUICallback = getGUIDebugger(infos, specs, metricsInfos, controls, ios, ddsEnv, ddsCustomCmd);

  while (pollGUI(window, debugGUICallback)) {
    // Exit this loop if all the children say they want to quit.
    bool allReadyToQuit = true;
    for (auto &info : infos) {
      allReadyToQuit &= info.readyToQuit;
    }
    if (allReadyToQuit) {
      break;
    }
    // Display part. All you need to display should actually be in
    // `infos`.
    // TODO: split at \n
    // TODO: update this only once per 1/60 of a second or
    // things like this.
    // TODO: have multiple display modes
    // TODO: graphical view of the processing?
    assert(infos.size() == controls.size());
    std::smatch match;
    std::string token;
    const std::string delimiter("\n");
    for (size_t di = 0, de = infos.size(); di < de; ++di) {
      DeviceInfo &info = infos[di];
      DeviceControl &control = controls[di];
      DeviceMetricsInfo &metrics = metricsInfos[di];

      if (info.unprinted.empty()) {
        continue;
      }

      auto s = info.unprinted;
      size_t pos = 0;
      info.history.resize(info.historySize);

      while ((pos = s.find(delimiter)) != std::string::npos) {
          token = s.substr(0, pos);
          // Check if the token is a metric from SimpleMetricsService
          // if yes, we do not print it out and simply store it to be displayed
          // in the GUI.
          // Then we check if it is part of our Poor man control system
          // if yes, we execute the associated command.
          if (parseMetric(token, match)) {
            LOG(INFO) << "Found metric with key " << match[2]
                      << " and value " <<  match[4];
            processMetric(match, metrics);
          } else if (parseControl(token, match)) {
            auto command = match[1];
            auto validFor = match[2];
            LOG(INFO) << "Found control command " << command
                      << " valid for " << validFor;
            if (command == "QUIT") {
              if (validFor == "ALL") {
                for (auto &deviceInfo : infos) {
                  deviceInfo.readyToQuit = true;
                }
              }
            }
          } else if (!control.quiet && (strstr(token.c_str(), control.logFilter) != nullptr)) {
            assert(info.historyPos >= 0);
            assert(info.historyPos < info.history.size());
            info.history[info.historyPos] = token;
            info.historyPos = (info.historyPos + 1) % info.history.size();
            std::cout << "[" << info.pid() << "]: " << token << std::endl;
          }
          s.erase(0, pos + delimiter.length());
      }
      info.unprinted = s;
    }
    // FIXME: for the gui to work correctly I would actually need to
    //        run the loop more often and update whenever enough time has
    //        passed.
  }
  return 0;
}

int runDevice(int argc, char **argv, const o2::framework::DeviceSpec &spec) {
  std::cout << "Spawing new device " << spec.id
            << " in process with pid " << getpid() << std::endl;
  try {
    fair::mq::DeviceRunner runner{argc, argv};

    // Populate options from the command line. Notice that only the options
    // declared in the workflow definition are allowed.
    runner.AddHook<fair::mq::hooks::SetCustomCmdLineOptions>([&spec](fair::mq::DeviceRunner& r){
      boost::program_options::options_description optsDesc;
      populateBoostProgramOptions(optsDesc, spec.options);
      r.fConfig.AddToCmdLineOptions(optsDesc, true);
    });

    // We initialise this in the driver, because different drivers might have
    // different versions of the service
    ServiceRegistry serviceRegistry;
    serviceRegistry.registerService<MetricsService>(new SimpleMetricsService());
    serviceRegistry.registerService<RootFileService>(new LocalRootFileService());
    serviceRegistry.registerService<ControlService>(new TextControlService());

    std::unique_ptr<FairMQDevice> device;
    if (spec.inputs.empty()) {
      LOG(DEBUG) << spec.id << " is a source\n";
      device.reset(new DataSourceDevice(spec, serviceRegistry));
    } else {
      LOG(DEBUG) << spec.id << " is a processor\n";
      device.reset(new DataProcessingDevice(spec, serviceRegistry));
    }

    runner.AddHook<fair::mq::hooks::InstantiateDevice>([&device](fair::mq::DeviceRunner& r){
      r.fDevice = std::shared_ptr<FairMQDevice>{std::move(device)};
    });

    return runner.Run();
  }
  catch(std::exception &e) {
    LOG(ERROR) << "Unhandled exception reached the top of main: " << e.what() << ", device shutting down.";
    return 1;
  }
  catch(...) {
    LOG(ERROR) << "Unknown exception reached the top of main.\n";
    return 1;
  }
  return 0;
}

void verifyWorkflow(const o2::framework::WorkflowSpec &specs) {
  std::set<std::string> validNames;
  std::vector<OutputSpec> availableOutputs;
  std::vector<InputSpec> requiredInputs;

  // An index many to one index to go from a given input to the
  // associated spec
  std::map<size_t, size_t> inputToSpec;
  // A one to one index to go from a given output to the Spec emitting it
  std::map<size_t, size_t> outputToSpec;

  for (auto &spec : specs)
  {
    if (spec.name.empty())
      throw std::runtime_error("Invalid DataProcessorSpec name");
    if (validNames.find(spec.name) != validNames.end())
      throw std::runtime_error("Name " + spec.name + " is used twice.");
    for (auto &option : spec.options) {
      if (option.type != option.defaultValue.type()) {
        std::ostringstream ss;
        ss << "Mismatch between declared option type and default value type"
           << "for " << option.name << " in DataProcessorSpec of "
           << spec.name;
        throw std::runtime_error(ss.str());
      }
    }
  }
}

static void handle_sigint(int signum) {
  // We kill ourself (SPOOKY!)
  signal(SIGINT, SIG_DFL);
  kill(getpid(), SIGINT);
}

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

// This is a toy executor for the workflow spec
// What it needs to do is:
//
// - Print the properties of each DataProcessorSpec
// - Generate DDS topology from DataProcessorSpec
// - Start DDS
// - Activate DDS topology
// - On GUI shutdown, stop DDS topology and stop dds-server
int doMain(int argc, char **argv, const o2::framework::WorkflowSpec & specs) {
  static struct option longopts[] = {
    {"quiet",     no_argument,  nullptr, 'q' },
    {"id", required_argument, nullptr, 'i'},
    { nullptr,         0,            nullptr, 0 }
  };

  bool defaultQuiet = false;
  std::string frameworkId;

  int opt;
  size_t safeArgsSize = sizeof(char**)*argc+1;
  char **safeArgv = reinterpret_cast<char**>(malloc(safeArgsSize));
  memcpy(safeArgv, argv, safeArgsSize);

  while ((opt = getopt_long(argc, argv, "qi",longopts, nullptr)) != -1) {
    switch (opt) {
    case 'q':
        defaultQuiet = true;
        break;
    case 'i':
        frameworkId = optarg;
        break;
    case ':':
    case '?':
    default: /* '?' */
        // By default we ignore all the other options: we assume they will be
        // need by on of the underlying devices.
        break;
    }
  }

  std::vector<DeviceSpec> deviceSpecs;

  LOG(INFO) << "Verifying workflow";
  try {
    verifyWorkflow(specs);
    dataProcessorSpecs2DeviceSpecs(specs, deviceSpecs);
    // This should expand nodes so that we can build a consistent DAG.
  } catch (std::runtime_error &e) {
    LOG(ERROR) << "Invalid workflow: " << e.what();
    return 1;
  }

  // Up to here, parent and child need to do exactly the same thing. After, we
  // distinguish between something which has a framework id (the children) and something
  // which does not, the parent.
  if (frameworkId.empty() == false) {
    for (auto &spec : deviceSpecs) {
      if (spec.id == frameworkId) {
        return runDevice(argc, safeArgv, spec);
      }
    }
    LOG(ERROR) << "Unable to find component with id" << frameworkId;
  }

  assert(frameworkId.empty());

  gDeviceControls.resize(deviceSpecs.size());
  prepareArguments(argc, argv, defaultQuiet,
                   deviceSpecs, gDeviceControls);

  LOG(INFO) << "Generating o2-dds-topology.xml";
  std::ofstream file{"o2-dds-topology.xml", std::ios::out | std::ios::trunc};
  deviceSpecs2DDSTopology(file, deviceSpecs);
  file.close();

  auto env = boost::this_process::environment();
  bp::environment dds_env = env;

  // Setup DDS environment
  LOG(INFO) << "Setting up DDS environment";
  {
    if (env["DDS_ROOT"].empty()) {
      LOG(ERROR) << "Could not find DDS installation in environment variable DDS_ROOT. Exiting.";
      exit(1);
    }

    auto old_pwd = bfs::current_path();
    bfs::current_path(env["DDS_ROOT"].to_vector()[0]);

    bp::ipstream is; //reading pipe-stream
    bp::child c("/bin/bash -c \"source ./DDS_env.sh > /dev/null 2>&1 && env\"", bp::std_out > is);

    std::string line;
    std::vector<std::string> elems;
    // preserve environment of source ./DDS_env.sh
    while (std::getline(is, line) && !line.empty()) {
      split(line, '=', std::back_inserter(elems));
      if (elems.size() == 2) {
        // we need dds environment for current process(intercom_api) and for shellouts
        dds_env[elems[0]] = elems[1];
        env[elems[0]] = elems[1];
      }
      elems.clear();
    }
    c.wait();

    bfs::current_path(old_pwd);
  }

  // Start DDS server
  LOG(INFO) << "Starting DDS server";
  {
    bp::ipstream is; //reading pipe-stream
    bp::child c(bp::search_path("dds-server"), "start", "-s", bp::std_out > is, dds_env);

    std::string line;
    while (std::getline(is, line)) {
      LOG(DEBUG) << line;
    }
    c.wait();
  }

  basio::io_service ios;
  basio::io_service::work work(ios);
  std::thread uiWorker([&](){ ios.run(); });

  // Submit DDS agents
  LOG(INFO) << "Submitting " << deviceSpecs.size() << " DDS agents to localhost";
  {
    bp::ipstream is; //reading pipe-stream
    bp::child c(bp::search_path("dds-submit"), "--rms", "localhost", "-n", std::to_string(deviceSpecs.size()), bp::std_out > is, dds_env);

    std::string line;
    while (std::getline(is, line)) {
      LOG(DEBUG) << line;
    }
    c.wait();
  }

  LOG(INFO) << "Retrieve DDS agents";
  {
    bp::ipstream is; //reading pipe-stream
    bp::child c(bp::search_path("dds-info"), "-l", bp::std_out > is, dds_env);

    std::string line;
    while (std::getline(is, line)) {
      LOG(DEBUG) << line;
    }
    c.wait();
  }

  dds::intercom_api::CIntercomService ddsIntercomService;
  dds::intercom_api::CCustomCmd ddsCustomCmd{ddsIntercomService};

  ddsCustomCmd.subscribe([&](const std::string& cmd, const std::string& cond, uint64_t senderId) {
    std::regex heartbeat_regex{"^heartbeat: ([^\\s,]+),(\\d+)"};
    std::smatch heartbeat_match;
    std::regex state_regex{"^state-change: ([^\\s,]+),([\\S ]+)"};
    std::smatch state_match;

    if (std::regex_match(cmd, heartbeat_match, heartbeat_regex)) {
      // FIXME: linear search too expensive
      for (int i = 0; i < deviceSpecs.size(); ++i) {
        if (deviceSpecs[i].id == heartbeat_match[1].str()) {
          DeviceInfo &info = gDeviceInfos[i];
          info.pid(std::stoi(heartbeat_match[2].str()));
          info.withLock([&](){
            if (info.stateUnsafe() == DeviceState::Disconnected) {
              info.stateUnsafe(DeviceState::Connected);
            }
          });
        }
      }
    } else if (std::regex_match(cmd, state_match, state_regex)) {
      LOG(DEBUG) << cmd;
      // FIXME: linear search too expensive
      for (int i = 0; i < deviceSpecs.size(); ++i) {
        if (deviceSpecs[i].id == state_match[1].str()) {
          DeviceInfo &info = gDeviceInfos[i];
          auto newState = state_match[2].str();
          if (newState == "RUNNING") {
            info.state(DeviceState::Running);
          } else if (newState == "DEVICE READY") {
            info.state(DeviceState::DeviceReady);
          } else if (newState == "READY") {
            info.state(DeviceState::Ready);
          } else if (newState == "INITIALIZING TASK") {
            info.state(DeviceState::InitializingTask);
          } else if (newState == "INITIALIZING DEVICE") {
            info.state(DeviceState::InitializingDevice);
          } else if (newState == "RESETTING TASK") {
            info.state(DeviceState::ResettingTask);
          } else if (newState == "RESETTING DEVICE") {
            info.state(DeviceState::ResettingDevice);
          } else if (newState == "IDLE") {
            info.state(DeviceState::Idle);
          } else if (newState == "EXITING") {
            info.state(DeviceState::Exiting);
          }
        }
      }
    } else {
      LOG(INFO) << "Received unknown command: " << cmd;
    }
  });

  ddsIntercomService.start();

  for (size_t di = 0; di < deviceSpecs.size(); ++di) {
    gDeviceInfos.emplace_back(true, false, 1000, 0);
    // Let's add also metrics information for the given device
    gDeviceMetricsInfos.emplace_back(DeviceMetricsInfo{});
  }

  auto result = doGUI(gDeviceInfos,
                      deviceSpecs,
                      gDeviceControls,
                      gDeviceMetricsInfos,
                      ios,
                      dds_env,
                      ddsCustomCmd);

  work.~work();
  uiWorker.join();

  // Stop DDS server
  LOG(INFO) << "Shutting down DDS cluster and DDS server";
  {
    bp::ipstream is; //reading pipe-stream
    bp::child c(bp::search_path("dds-server"), "stop", bp::std_out > is, dds_env);

    std::string line;
    while (std::getline(is, line)) {
      LOG(DEBUG) << line;
    }
    c.wait();
  }

  return result;
}
