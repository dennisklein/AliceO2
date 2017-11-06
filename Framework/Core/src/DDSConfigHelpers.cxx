// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "DDSConfigHelpers.h"
#include <map>
#include <iostream>
#include <string>

namespace o2 {
namespace framework {

void
deviceSpecs2DDSTopology(std::ostream &out, const std::vector<DeviceSpec> &specs)
{
  out << "<topology id=\"o2-dataflow\">" << std::endl;
  for (auto &spec : specs) {
    for (const auto& cs : spec.channels) {
      if (cs.method == ChannelMethod::Bind) {
        out << "  <property id=\"" << cs.name << "\" />" << std::endl;
      }
    }
  }
  out << std::endl;
  for (auto &spec : specs) {
    auto id = spec.id;
    std::replace(id.begin(), id.end(), '-', '_'); // replace all 'x' to 'y'
    out << "  <decltask id=\"" << id << "\">" << std::endl;
    out << "    <exe reachable=\"true\"><![CDATA[/usr/bin/bash -c \"eval `alienv load O2/latest` && ";
    for (size_t ai = 0; ai < spec.args.size(); ++ai) {
      const char *arg = spec.args[ai];
      if (!arg) { break; }
      out << arg << " ";
    }
    out << "\"]]></exe>" << std::endl;
    out << "    <properties>" << std::endl;
    for (const auto& cs : spec.channels) {
      out << "      <id access=\"" << std::string{(cs.method == ChannelMethod::Bind) ? "write" : "read"}<< "\">" << cs.name << "</id>" << std::endl;
    }
    out << "    </properties>" << std::endl;
    out << "  </decltask>" << std::endl;
  }
  out << std::endl;
  out << "  <main id=\"main\">" << std::endl;
  for (auto &spec : specs) {
    out << "    <task>" << spec.id << "</task>" << std::endl;
  }
  out << "  </main>" << std::endl;
  out << "</topology>\n";
}

} // namespace framework
} // namespace o2
