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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sstmac.h"
#include "sstmac_av.h"

#include <sprockit/errors.h>

EXTERN_C DIRECT_FN STATIC  int sstmac_setname(fid_t fid, void *addr, size_t addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_getname(fid_t fid, void *addr, size_t *addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_getpeer(struct fid_ep *ep, void *addr, size_t *addrlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_connect(struct fid_ep *ep, const void *addr,
				  const void *param, size_t paramlen);
EXTERN_C DIRECT_FN STATIC  int sstmac_accept(struct fid_ep *ep, const void *param,
				 size_t paramlen);
extern "C" DIRECT_FN  int sstmac_pep_open(struct fid_fabric *fabric,
			    struct fi_info *info, struct fid_pep **pep,
			    void *context);
extern "C" DIRECT_FN  int sstmac_pep_bind(struct fid *fid, struct fid *bfid, uint64_t flags);
EXTERN_C DIRECT_FN STATIC  int sstmac_reject(struct fid_pep *pep, fid_t handle,
				 const void *param, size_t paramlen);
DIRECT_FN STATIC int sstmac_shutdown(struct fid_ep *ep, uint64_t flags);
extern "C" DIRECT_FN  int sstmac_pep_listen(struct fid_pep *pep);
EXTERN_C DIRECT_FN STATIC  int sstmac_pep_getopt(fid_t fid, int level, int optname,
             void *optval, size_t *optlen);
static int sstmac_pep_close(fid_t fid);

struct fi_ops_cm sstmac_ep_ops_cm = {
  .size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = sstmac_getpeer,
  .connect = fi_no_connect,
  .listen = fi_no_listen,
  .accept = fi_no_accept,
  .reject = fi_no_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};

struct fi_ops_cm sstmac_ep_msg_ops_cm = {
	.size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = sstmac_getpeer,
  .connect = sstmac_connect,
	.listen = fi_no_listen,
  .accept = sstmac_accept,
	.reject = fi_no_reject,
  .shutdown = sstmac_shutdown,
	.join = fi_no_join,
};

struct fi_ops sstmac_pep_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_pep_close,
  .bind = sstmac_pep_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open,
};

struct fi_ops_ep sstmac_pep_ops_ep = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = fi_no_cancel,
  .getopt = sstmac_pep_getopt,
  .setopt = fi_no_setopt,
  .tx_ctx = fi_no_tx_ctx,
  .rx_ctx = fi_no_rx_ctx,
  .rx_size_left = fi_no_rx_size_left,
  .tx_size_left = fi_no_tx_size_left,
};

struct fi_ops_cm sstmac_pep_ops_cm = {
  .size = sizeof(struct fi_ops_cm),
  .setname = sstmac_setname,
  .getname = sstmac_getname,
  .getpeer = fi_no_getpeer,
  .connect = fi_no_connect,
  .listen = sstmac_pep_listen,
  .accept = fi_no_accept,
  .reject = sstmac_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};

EXTERN_C DIRECT_FN STATIC  int sstmac_getname(fid_t fid, void *addr, size_t *addrlen)
{
  sstmac_fid_ep* ep = (sstmac_fid_ep*) fid;
  int input_size = *addrlen;
  if (ep->domain->addr_format == FI_ADDR_STR){
    *addrlen = snprintf((char*)addr, *addrlen, "%" PRIu64, ep->src_addr);
    if (*addrlen > input_size){
      return -FI_ETOOSMALL;
    }
  } else if (ep->domain->addr_format == FI_ADDR_SSTMAC){
    *addrlen = sizeof(fi_addr_t);
    if (input_size < sizeof(fi_addr_t)){
      return -FI_ETOOSMALL;
    }
  } else {
    warn_einval("got bad addr format");
  }

  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_setname(fid_t fid, void *addr, size_t addrlen)
{
  sstmac_fid_ep* ep = (sstmac_fid_ep*) fid;
  if (!addr){
    warn_einval("got null input addr");
    return -FI_EINVAL;
  }

  fi_addr_t* addr_to_change = nullptr;
  switch(fid->fclass){
  case FI_CLASS_EP:
    addr_to_change = &ep->src_addr;
    break;
  case FI_CLASS_SEP:
    warn_einval("scalable endpoints not yet supported for sstmac provider");
    return -FI_EINVAL;
  case FI_CLASS_PEP:
    warn_einval("passive endpoints not yet supported for sstmac provider");
    return -FI_EINVAL;
  }


  // no need to worry about FI_ADDR_STR here
  if (addrlen < sizeof(fi_addr_t)){
    warn_einval("addrlen %zu is too small to hold fi_addr_t", addrlen);
    return -FI_EINVAL;
  }
  *addr_to_change = *((fi_addr_t*)addr);
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_getpeer(struct fid_ep *ep, void *addr,
				  size_t *addrlen)
{
  sstmac_fid_ep* ep_impl = (sstmac_fid_ep*) ep;
  int input_size = *addrlen;
  *addrlen =  sizeof(fi_addr_t);
  if (*addrlen < sizeof(fi_addr_t)){
    warn_einval("addrlen %d is too small to hold fi_addr_t", input_size);
    return -FI_ETOOSMALL;
  }

  fi_addr_t* addr_ptr = (fi_addr_t*) addr;
  *addr_ptr = ep_impl->dest_addr;
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_connect(struct fid_ep */*ep*/, const void */*addr*/,
          const void */*param*/, size_t /*paramlen*/)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_accept(struct fid_ep */*ep*/, const void */*param*/,
         size_t /*paramlen*/)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC int sstmac_shutdown(struct fid_ep *ep, uint64_t flags)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC int sstmac_reject(struct fid_pep *pep, fid_t handle,
       const void *param, size_t paramlen)
{
  return -FI_ENOSYS;
}


/******************************************************************************
 *
 * Passive endpoint handling
 *
 *****************************************************************************/

EXTERN_C DIRECT_FN STATIC  int sstmac_pep_getopt(fid_t /*fid*/, int /*level*/, int /*optname*/,
             void */*optval*/, size_t */*optlen*/)
{
  return -FI_ENOSYS;
}

static int sstmac_pep_close(fid_t /*fid*/)
{
  return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sstmac_pep_bind(struct fid */*fid*/, struct fid */*bfid*/, uint64_t /*flags*/)
{
  return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sstmac_pep_listen(struct fid_pep */*pep*/)
{
  return -FI_ENOSYS;
}


extern "C" DIRECT_FN  int sstmac_pep_open(struct fid_fabric */*fabric*/,
          struct fi_info */*info*/, struct fid_pep **/*pep*/,
          void */*context*/)
{
  return -FI_ENOSYS;
}



