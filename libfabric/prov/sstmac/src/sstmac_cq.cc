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
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sstmac.h"
#include "sstmac_wait.h"

#include <sstmac_sumi.hpp>

static int sstmac_cq_close(fid_t fid);
static int sstmac_cq_control(struct fid *cq, int command, void *arg);
DIRECT_FN STATIC ssize_t sstmac_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr);
DIRECT_FN STATIC ssize_t sstmac_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout);
DIRECT_FN extern "C" int sstmac_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context);
DIRECT_FN STATIC const char *sstmac_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len);
DIRECT_FN STATIC extern "C" int sstmac_cq_signal(struct fid_cq *cq);
DIRECT_FN STATIC ssize_t sstmac_cq_read(struct fid_cq *cq,
				      void *buf,
				      size_t count);
DIRECT_FN STATIC ssize_t sstmac_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout);

static const struct fi_ops sstmac_cq_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_cq_close,
  .bind = fi_no_bind,
  .control = sstmac_cq_control,
  .ops_open = fi_no_ops_open
};

static const struct fi_ops_cq sstmac_cq_ops = {
  .size = sizeof(struct fi_ops_cq),
  .read = sstmac_cq_read,
  .readfrom = sstmac_cq_readfrom,
  .readerr = sstmac_cq_readerr,
  .sread = sstmac_cq_sread,
  .sreadfrom = sstmac_cq_sreadfrom,
  .signal = sstmac_cq_signal,
  .strerror = sstmac_cq_strerror
};

static const size_t format_sizes[] = {
  [FI_CQ_FORMAT_UNSPEC]  = sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_CONTEXT] = sizeof(struct fi_cq_entry),
	[FI_CQ_FORMAT_MSG]     = sizeof(struct fi_cq_msg_entry),
	[FI_CQ_FORMAT_DATA]    = sizeof(struct fi_cq_data_entry),
	[FI_CQ_FORMAT_TAGGED]  = sizeof(struct fi_cq_tagged_entry)
};


static void* sstmaci_fill_cq_entry(enum fi_cq_format format, sumi::Message* msg, void* buf, void* context)
{
  FabricMessage* fmsg = static_cast<FabricMessage*>(msg);
  switch (format){
    case FI_CQ_FORMAT_UNSPEC:
    case FI_CQ_FORMAT_CONTEXT: {
      fi_cq_entry* entry = (fi_cq_entry*) buf;
      entry->op_context = context;
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_MSG: {
      fi_cq_msg_entry* entry = (fi_cq_msg_entry*) buf;
      entry->op_context = context;
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_DATA: {
      fi_cq_data_entry* entry = (fi_cq_data_entry*) buf;
      entry->op_context = context;
      entry->buf = msg->localBuffer();
      entry->data = fmsg->immData();
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry++;
      return entry;
    }
    case FI_CQ_FORMAT_TAGGED: {
      fi_cq_tagged_entry* entry = (fi_cq_tagged_entry*) buf;
      entry->op_context = context;
      entry->buf = msg->localBuffer();
      entry->data = fmsg->immData();
      entry->len = msg->byteLength();
      entry->flags = fmsg->flags();
      entry->tag = fmsg->tag();
      entry++;
      return entry;
    }
  }
  return nullptr;
}




struct sstmac_fid_cq {
  struct fid_cq cq_fid;
  struct sstmac_fid_domain *domain;
  int id; //the sumi CQ id allocated to this
  struct fi_cq_attr attr;
  size_t entry_size;
  struct fid_wait *wait;
};

static int sstmac_cq_close(fid_t fid)
{
  sstmac_fid_cq* cq_impl = (sstmac_fid_cq*) fid;
  FabricTransport* tport = (FabricTransport*) cq_impl->domain->fabric->tport;
  tport->deallocateCq(cq_impl->id);
  free(cq_impl);
	return FI_SUCCESS;
}

static ssize_t sstmaci_cq_read(bool blocking,
                        struct fid_cq *cq, void *buf,
                        size_t count, fi_addr_t *src_addr,
                        const void *cond, int timeout)
{
  sstmac_fid_cq* cq_impl = (sstmac_fid_cq*) cq;
  FabricTransport* tport = (FabricTransport*) cq_impl->domain->fabric->tport;

  size_t done = 0;
  while (done < count){
    double timeout_s = (blocking && timeout > 0) ? timeout*1e-3 : -1;
    sumi::Message* msg = tport->poll(blocking, cq_impl->id, timeout_s);
    if (!msg){
      break;
    }
    if (src_addr){
      src_addr[done] = msg->sender();
    }
    //buf = sstmaci_fill_cq_entry(cq_impl->, msg, buf,)
    done++;
  }
  return done ? done : -FI_EAGAIN;
}

DIRECT_FN STATIC ssize_t sstmac_cq_sreadfrom(struct fid_cq *cq, void *buf,
					   size_t count, fi_addr_t *src_addr,
					   const void *cond, int timeout)
{
  return sstmaci_cq_read(true, cq, buf, count, src_addr, cond, timeout);
}

DIRECT_FN STATIC ssize_t sstmac_cq_read(struct fid_cq *cq,
                void *buf, size_t count)
{
  return sstmaci_cq_read(false, cq, buf, count, nullptr, nullptr, -1);
}

DIRECT_FN STATIC ssize_t sstmac_cq_sread(struct fid_cq *cq, void *buf,
				       size_t count, const void *cond,
				       int timeout)
{
  return sstmaci_cq_read(true, cq, buf, count, nullptr, cond, timeout);
}

DIRECT_FN STATIC ssize_t sstmac_cq_readfrom(struct fid_cq *cq, void *buf,
					  size_t count, fi_addr_t *src_addr)
{
  return sstmaci_cq_read(false, cq, buf, count, src_addr, nullptr, -1);
}

DIRECT_FN STATIC ssize_t sstmac_cq_readerr(struct fid_cq *cq,
					 struct fi_cq_err_entry *buf,
					 uint64_t flags)
{
  //there are never any errors in the simulator!
  return -FI_EINVAL;
}

DIRECT_FN STATIC const char *sstmac_cq_strerror(struct fid_cq *cq, int prov_errno,
					      const void *prov_data, char *buf,
					      size_t len)
{
	return NULL;
}

DIRECT_FN STATIC extern "C" int sstmac_cq_signal(struct fid_cq *cq)
{
  spkt_abort_printf("unimplemented: ssmac_cq_signal");
	return FI_SUCCESS;
}

static int sstmac_cq_control(struct fid *cq, int command, void *arg)
{

	switch (command) {
	case FI_GETWAIT:
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
}

DIRECT_FN extern "C" int sstmac_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
			   struct fid_cq **cq, void *context)
{
  sstmac_fid_domain* domain_impl = (sstmac_fid_domain*) domain;
  FabricTransport* tport = (FabricTransport*) domain_impl;
  int id = tport->allocateCqId();
  sstmac_fid_cq* cq_impl = (sstmac_fid_cq*) calloc(1, sizeof(sstmac_fid_cq));
  cq_impl->domain = domain_impl;
  cq_impl->id = id;

  //I don't really care what this is, per se
  cq_impl->attr = *attr;

  struct fi_wait_attr requested = {
    .wait_obj = attr->wait_obj,
    .flags = 0
  };

  switch (attr->wait_obj) {
  case FI_WAIT_UNSPEC:
  case FI_WAIT_FD:
  case FI_WAIT_MUTEX_COND:
    sstmac_wait_open(&cq_impl->domain->fabric->fab_fid, &requested, &cq_impl->wait);
    break;
  case FI_WAIT_SET:
    sstmaci_add_wait(cq_impl->attr.wait_set, &cq_impl->cq_fid.fid);
    break;
  default:
    break;
  }
  *cq = (fid_cq*) cq_impl;
  return FI_SUCCESS;
}

