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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sprockit/errors.h>

#include "sstmac.h"
#include "sstmac_av.h"

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
            uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

EXTERN_C DIRECT_FN STATIC  int sstmac_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags);

EXTERN_C DIRECT_FN STATIC  int sstmac_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen);

DIRECT_FN const char *sstmac_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len);

static int sstmac_av_close(fid_t fid);

static struct fi_ops_av sstmac_av_ops = {
  .size = sizeof(struct fi_ops_av),
  .insert = sstmac_av_insert,
  .insertsvc = sstmac_av_insertsvc,
  .insertsym = sstmac_av_insertsym,
  .remove = sstmac_av_remove,
  .lookup = sstmac_av_lookup,
  .straddr = sstmac_av_straddr
};

static struct fi_ops sstmac_fi_av_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_av_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open
};


EXTERN_C DIRECT_FN STATIC  int sstmac_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen)
{
  sstmac_fid_av* av_impl = (sstmac_fid_av*) av;
  int input_size = *addrlen;
  if (av_impl->domain->addr_format == FI_ADDR_SSTMAC){
    *addrlen = sizeof(fi_addr_t);
    if (input_size >= sizeof(fi_addr_t)){
      uint64_t* addr_int = (uint64_t*) addr;
      *addr_int = fi_addr;
    }
  } else if (av_impl->domain->addr_format == FI_ADDR_STR){
    // we don't check if buf is too small, we just truncate
    *addrlen = sstmaci_fi_addr_to_str(fi_addr, (char*)addr, *addrlen);
  } else {
    warn_einval("got addr format that isn't FI_ADDR_SSTMAC or FI_ADDR_STR");
    return -FI_EINVAL;
  }

  if (*addrlen > input_size){
    return -FI_ETOOSMALL;
  } else {
    return FI_SUCCESS;
  }
}

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
				    uint64_t flags, void *context)
{
  sstmac_fid_av* av_impl = (sstmac_fid_av*) av;
  if (av_impl->domain->addr_format == FI_ADDR_STR){
    char* addr_str = (char*) addr;
    for (int i=0; i < count; ++i){
      long long rank = std::atoll(addr_str);
      fi_addr[i] = rank;
      addr_str += SSTMAC_MAX_ADDR_LEN;
    }
  } else if (av_impl->domain->addr_format == FI_ADDR_SSTMAC) {
    uint64_t* addr_list = (uint64_t*) addr;
    for (int i=0; i < count; ++i){
      fi_addr[i] = addr_list[i];
    }
  } else {
    warn_einval("got addr format that isn't SSTMAC or STR");
    return -FI_EINVAL;
  }
  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags)
{
  //we don't need to do anything to remove stuff
  return FI_SUCCESS;
}

DIRECT_FN const char *sstmac_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len)
{
  sstmac_fid_av* av_impl = (sstmac_fid_av*) av;
  char* ret = new char[SSTMAC_MAX_ADDR_LEN];

  if (av_impl->domain->addr_format == FI_ADDR_STR){
    ::strcpy(ret, (const char*)addr);
  } else if (av_impl->domain->addr_format == FI_ADDR_SSTMAC) {
    uint64_t* addr_ptr = (uint64_t*) addr;
    uint32_t rank = GET_SSTMAC_ADDR_RANK(*addr_ptr);
    uint16_t cq = GET_SSTMAC_ADDR_CQ(*addr_ptr);
    sprintf(ret, "%" PRIu32 ".%" PRIu16, rank, cq);
  } else {
    warn_einval("got addr format that isn't FI_ADDR_SSTMAC or FI_ADDR_STR");
    ret[0] = '\0';
  }
  *len = ::strlen(ret);
  return ret;
}

static int sstmac_av_close(fid_t fid)
{
  sstmac_fid_av* av_impl = (sstmac_fid_av*) fid;
  free(av_impl);
  return FI_SUCCESS;
}

extern "C" DIRECT_FN  int sstmac_av_bind(struct fid_av *av, struct fid *fid, uint64_t flags)
{
	return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sstmac_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
			   struct fid_av **av, void *context)
{
  sstmac_fid_av* av_impl = (sstmac_fid_av*) calloc(1, sizeof(sstmac_fid_av));
  av_impl->av_fid.fid.fclass = FI_CLASS_AV;
  av_impl->av_fid.fid.context = context;
  av_impl->av_fid.fid.ops = &sstmac_fi_av_ops;
  av_impl->av_fid.ops = &sstmac_av_ops;
  av_impl->domain = (sstmac_fid_domain*) domain;
  *av = (fid_av*) av_impl;
  return FI_SUCCESS;
}
