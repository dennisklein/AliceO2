// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#ifndef FRAMEWORK_DEVICEINFO_H
#define FRAMEWORK_DEVICEINFO_H

#include "Framework/Variant.h"

#include <vector>
#include <string>
#include <cstddef>
// For pid_t
#include <unistd.h>
#include <array>
#include <mutex>
#include <functional>

namespace o2 {
namespace framework {

enum class DeviceState {
  Disconnected,
  Connected,
  Idle,
  InitializingDevice,
  DeviceReady,
  InitializingTask,
  Ready,
  Running,
  ResettingTask,
  ResettingDevice,
  Exiting
};

struct DeviceInfo {
  size_t historyPos;
  size_t historySize;
  std::vector<std::string> history;
  std::string unprinted;
  bool active;
  bool readyToQuit;

  DeviceInfo(bool _active, bool _readyToQuit, size_t _historySize, size_t _historyPos)
  : _state{DeviceState::Disconnected}
  , _pid{0}
  , active{_active}
  , readyToQuit{_readyToQuit}
  , historySize{_historySize}
  , historyPos{_historyPos}
  {}

  DeviceInfo(DeviceInfo&& rhs)
  : _state{std::move(rhs._state)}
  , _pid{std::move(rhs._pid)}
  , active{std::move(rhs.active)}
  , readyToQuit{std::move(rhs.readyToQuit)}
  , historySize{std::move(rhs.historySize)}
  , historyPos{std::move(rhs.historyPos)}
  , history{std::move(rhs.history)}
  , unprinted{std::move(rhs.unprinted)}
  {}

  // TODO Find a better pattern for the following
  auto withLock(std::function<void(void)> f) -> void {
    std::lock_guard<std::mutex> lock{mutex};
    f();
  }
  auto state() const -> DeviceState {
    std::lock_guard<std::mutex> lock{mutex};
    return _state;
  }
  auto state(DeviceState newState) -> void {
    std::lock_guard<std::mutex> lock{mutex};
    _state = newState;
  }
  auto stateUnsafe() const -> DeviceState {
    return _state;
  }
  auto stateUnsafe(DeviceState newState) -> void {
    _state = newState;
  }
  auto pid() const -> pid_t {
    std::lock_guard<std::mutex> lock{mutex};
    return _pid;
  }
  auto pid(pid_t newPid) -> void {
    std::lock_guard<std::mutex> lock{mutex};
    _pid = newPid;
  }
  auto pidUnsafe() const -> pid_t {
    return _pid;
  }
  auto pidUnsafe(pid_t newPid) -> void {
    _pid = newPid;
  }

  private:
  mutable std::mutex mutex;
  DeviceState _state; 
  pid_t _pid;
};


} // namespace framework
} // namespace o2
#endif // FRAMEWORK_DEVICEINFO_H
