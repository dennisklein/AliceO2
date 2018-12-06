// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @author Sandro Wenzel

#include <iostream>
#include <thread>
#include <stdlib.h>
// #include <SimConfig/SimConfig.h>
#include "CommonUtils/ShmManager.h"
#include <string.h>

int main(int argc, char* argv[])
{
  int nworkers = std::thread::hardware_concurrency() / 2;
  auto f = getenv("ALICE_NSIMWORKERS");
  if (f) {
    nworkers = atoi(f);
  }

  if (strncmp(argv[1], "nworkers",8) == 0) {
    std::cout << nworkers << std::endl;
    return 0;
  }

  if (strncmp(argv[1],"shm_create", 8) == 0) {
    o2::utils::ShmManager::Instance().createGlobalSegment(nworkers);
    return 0;
  }
  
  if (strncmp(argv[1], "shm_status", 8) == 0) {
    if (o2::utils::ShmManager::Instance().isOperational()) {
      return 0;
    } else {
      return 1;
    }
  }

  if (strncmp(argv[1],"shm_cleanup", 8) == 0) {
    o2::utils::ShmManager::Instance().release();
    return 0;
  }

  // if (argv[1] == "config") {
    // auto& conf = o2::conf::SimConfig::Instance();
    // if (!conf.resetFromArguments(argc, argv)) {
      // return 1;
    // }
    // std::cout << conf.c_str() << std::endl;
    // return 0;
  // }

  std::cout << std::endl;
  std::cout << "Usage: o2sim_parallel_tool [CMD]" << std::endl;
  std::cout << std::endl;
  std::cout << "CMD:" << std::endl;
  std::cout << " nworkers        - Print number of sim workers." << std::endl;
  std::cout << " shm_create      - Create shm segment." << std::endl;
  std::cout << " shm_status      - Check if shm segement is operational." << std::endl;
  std::cout << " shm_cleanup     - Remove shm segment." << std::endl;
  std::cout << std::endl;
  std::cout << "else: Print this usage information." << std::endl;
  std::cout << std::endl;
  return 0;
}
