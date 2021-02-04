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

#ifndef SSTMAC_SUMI_HPP
#define SSTMAC_SUMI_HPP

#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <sprockit/errors.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sumi/sim_transport.h>
#include <rdma/fabric.h>
#include <sstmac/common/stats/stat_collector.h>

class FabricMessage : public sumi::Message {
 public:
  constexpr static uint64_t no_tag = std::numeric_limits<uint64_t>::max();
  constexpr static uint64_t no_imm_data = std::numeric_limits<uint64_t>::max();

  template <class... Args>
  FabricMessage(uint64_t tag, uint64_t flags, uint64_t imm_data, void* ctx,
                Args&&... args) :
    sumi::Message(std::forward<Args>(args)...),
    tag_(tag),
    flags_(flags),
    imm_data_(imm_data),
    context_(ctx)
  {
    if (flags & FI_INJECT){
      size_t sz = byteLength();
      if (isNonNullBuffer(smsgBuffer())){
        ::memcpy(inject_data_, smsgBuffer(), sz);
        //clear the data buffer already there
        clearSmsgBuffer();
      }
    }
  }

  std::string toString() const override {
    return sprockit::sprintf("libfabric message tag=%" PRIu64 " flags=%" PRIu64 " context=%p, %s",
                             tag_, flags_, context_, sumi::Message::toString().c_str());
  }

  NetworkMessage* cloneInjectionAck() const override {
    auto* msg = new FabricMessage(*this);
    msg->convertToAck();
    return msg;
  }

  void matchRecv(void* buf){
    if (flags_ & FI_INJECT){
      if (isNonNullBuffer(buf)){
        ::memcpy(buf, inject_data_, byteLength());
      }
    }
    sumi::Message::matchRecv(buf);
  }

  uint64_t tag() const {
    return tag_;
  }

  uint64_t flags() const {
    return flags_;
  }

  uint64_t immData() const {
    return imm_data_;
  }

  void* context() const {
    return context_;
  }

  void setContext(void* ctx) {
    context_ = ctx;
  }


 private:
  uint64_t flags_;
  uint64_t imm_data_;
  uint64_t tag_;
  void* context_;
  char inject_data_[64];
};

class FabricTransport : public sumi::SimTransport {

 public:
  SST_ELI_REGISTER_DERIVED(
    API,
    FabricTransport,
    "macro",
    "libfabric",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the libfabric transport API")

  public:
    FabricTransport(SST::Params& params,
                    sstmac::sw::App* parent,
                    SST::Component* comp) :
      sumi::SimTransport(params, parent, comp),
      inited_(false)
  {
  }

  void init() override {
    sumi::SimTransport::init();
    inited_ = true;
  }

  bool inited() const {
    return inited_;
  }

  using DelayStat = SST::Statistics::MultiStatistic<int, //sender
  int, //recver
  uint64_t, //byte length
  double, //total delay
  double, //injection delay
  double, //contention delay
  double, //total network delay
  double //synchronization delay
 >;

  DelayStat* delayStat() const {
    return delays_;
  }

 private:
  bool inited_;
  std::vector<FabricMessage*> unmatched_recvs_;
  DelayStat* delays_;

};

FabricTransport* sstmac_fabric();

#endif // SSTMAC_SUMI_HPP
