/**
Copyright 2009-2020 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2020, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sstmac_sumi.hpp>
#include <sstmac/software/api/api.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>
#include "sstmac.h"

FabricTransport* sstmac_fabric()
{
  sstmac::sw::Thread* t = sstmac::sw::OperatingSystem::currentThread();
  FabricTransport* tp = t->getApi<FabricTransport>("libfabric");
  if (!tp->inited())
    tp->init();
  return tp;
}

FabricDelayStat::FabricDelayStat(SST::BaseComponent* comp, const std::string& name,
            const std::string& subName, SST::Params& params)
  : Parent(comp, name, subName, params)
{
}

void
FabricDelayStat::addData_impl(int src, int dst,
                             uint64_t bytes,
                             double total_delay,
                             double injection_delay, double contention_delay,
                             double total_network_delay, double sync_delay)
{
  messages_.emplace_back(src, dst, bytes, total_delay,
                         injection_delay, contention_delay, total_network_delay, sync_delay);
}


void
FabricDelayStat::registerOutputFields(SST::Statistics::StatisticFieldsOutput * /*statOutput*/)
{
  sprockit::abort("FabricDelayStat::registerOutputFields: should not be called");
}

void
FabricDelayStat::outputStatisticFields(SST::Statistics::StatisticFieldsOutput * /*output*/, bool  /*endOfSimFlag*/)
{
  sprockit::abort("FabricDelayStat::outputStatisticData: should not be called");
}

FabricDelayStatOutput::FabricDelayStatOutput(SST::Params& params) :
    sstmac::StatisticOutput(params)
{
}

void
FabricDelayStatOutput::startOutputGroup(sstmac::StatisticGroup *grp)
{
  auto outfile = grp->name + ".csv";
  out_.open(outfile);
  out_ << "component,src,dst,size,total_delay,injection,network,total_network,sync";
}

void
FabricDelayStatOutput::stopOutputGroup()
{
  out_.close();
}

void
FabricDelayStatOutput::output(SST::Statistics::StatisticBase* statistic, bool  /*endOfSimFlag*/)
{
  FabricDelayStat* stats = dynamic_cast<FabricDelayStat*>(statistic);
  for (auto iter=stats->begin(); iter != stats->end(); ++iter){
    const FabricDelayStat::Message& m = *iter;
    out_ << "\n" << stats->getStatSubId()
         << "," << m.src
         << "," << m.dst
         << "," << m.length
         << "," << m.total_delay
         << "," << m.inj_delay
         << "," << m.contention_delay
         << "," << m.total_network_delay
         << "," << m.sync_delay
    ;
  }
}

constexpr uint64_t FabricMessage::no_tag;
constexpr uint64_t FabricMessage::no_imm_data;
