#! /usr/bin/env bash

# Copyright CERN and copyright holders of ALICE O2. This software is
# distributed under the terms of the GNU General Public License v3 (GPL
# Version 3), copied verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

export ALICE_SIMFORKINTERNAL=1

source ${DDS_ROOT}/DDS_env.sh

dds-session start

sessionID=$(dds-user-defaults --default-session-id)
echo "SESSION ID: ${sessionID}"
wrkDir=$(dds-user-defaults -V --key server.work_dir)
echo "WORK DIR: ${wrkDir}"

topologyFile=$(eval echo "${wrkDir}/o2sim_parallel_dds_topology.xml")
requiredNofAgents=3

dds-submit -r localhost -n ${requiredNofAgents}

dds-info --wait-for-idle-agents ${requiredNofAgents}

cat << EOF > ${topologyFile}
<topology id="o2sim_parallel">

    <property id="primary-get" />
    <property id="simdata" />

    <decltask id="O2PrimaryServer">
        <exe reachable="true">${O2_ROOT}/bin/O2PrimaryServerDeviceRunner --id O2PrimaryServer --control static --plugin-search-path "&lt;${FAIRMQ_ROOT}/lib" --plugin dds --channel-config name=primary-get,type=rep,method=bind</exe>
        <properties>
            <id access="write">primary-get</id>
        </properties>
    </decltask>

    <decltask id="O2SimWorkerPool">
        <exe reachable="true">${O2_ROOT}/bin/O2SimDeviceRunner</exe>
        <properties>
            <id access="read">primary-get</id>
            <id access="read">simdata</id>
        </properties>
    </decltask>

    <decltask id="O2HitMerger">
        <exe reachable="true">${O2_ROOT}/bin/O2HitMergerRunner --id O2HitMerger --control static --plugin-search-path "&lt;${FAIRMQ_ROOT}/lib" --plugin dds --channel-config name=primary-get,type=req,method=connect --channel-config name=simdata,type=pull,method=bind</exe>
        <properties>
            <id access="read">primary-get</id>
            <id access="write">simdata</id>
        </properties>
    </decltask>

    <main id="main">
        <task>O2PrimaryServer</task>
        <task>O2SimWorkerPool</task>
        <task>O2HitMerger</task>
    </main>

</topology>
EOF

o2sim_parallel_tool shm_create

dds-topology --disable-validation --activate ${topologyFile}

dds-info -l

dds-info --wait-for-idle-agents 2

dds-topology --stop

dds-agent-cmd getlog -a

logDir=$(eval echo "${wrkDir}/log/agents")
for file in $(find "${logDir}" -name "*.tar.gz"); do tar -xf ${file} -C "${logDir}" ; done
echo "LOG FILES IN: ${logDir}"

#nofGoodResults=$(grep -r --include "*.log" "User task exited with status 0" "${logDir}" | wc -l)

#if [ "${nofGoodResults}" -eq "${requiredNofAgents}" ]
#then
#  echo "---=== SUCCESS ===---"
#else
#  echo "Test failed: number of good results is ${nofGoodResults}, required number of good results is ${requiredNofAgents}"
#fi

dds-session stop ${sessionID}

o2sim_parallel_tool shm_cleanup
