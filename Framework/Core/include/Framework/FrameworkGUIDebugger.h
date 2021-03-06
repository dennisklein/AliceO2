// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#ifndef FRAMEWORK_FRAMEWORKGUIDEBUGGER_H
#define FRAMEWORK_FRAMEWORKGUIDEBUGGER_H

#include "Framework/DeviceInfo.h"
#include "Framework/DeviceSpec.h"
#include "Framework/DeviceMetricsInfo.h"
#include "Framework/DeviceControl.h"
#include <functional>
#include <vector>
#include <boost/asio.hpp>
namespace basio = boost::asio;
#include <boost/process.hpp>
namespace bp = boost::process;
#include <mutex>
#include <dds_intercom.h>

namespace o2 {
namespace framework {

struct DeviceGUIState {
  std::string label;
};

enum class TopologyState {
    Start,
    Working,
    Activated,
    Resetted,
    Stopping
};

struct WorkspaceGUIState {
  int selectedMetric;
  std::vector<std::string> availableMetrics;
  std::vector<DeviceGUIState> devices;
};

struct TopologyControls {
  TopologyControls()
  : topology{TopologyState::Start} {}

  void SetTopologyState(TopologyState newState) {
    std::lock_guard<std::mutex> lock{topologyMutex};
    topology = newState;
  }
  TopologyState GetTopologyState() {
    std::lock_guard<std::mutex> lock{topologyMutex};
    return topology;
  }

  private:
  TopologyState topology;
  std::mutex topologyMutex;
};

static WorkspaceGUIState gState;
static TopologyControls gTopo;

std::function<void(void)> getGUIDebugger(const std::vector<DeviceInfo> &infos,
                                         const std::vector<DeviceSpec> &specs,
                                         const std::vector<DeviceMetricsInfo> &metricsInfos,
                                         std::vector<DeviceControl> &controls,
                                         basio::io_service& ios,
                                         bp::environment& ddsEnv,
                                         dds::intercom_api::CCustomCmd& ddsCustomCmd);

}
}
#endif // FRAMEWORK_FRAMEWORKGUIDEBUGGER_H
