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
#include <assert.h>

#include <stdlib.h>

#include "sstmac.h"
#include "sstmac_wait.h"

#include <vector>


#define GNIX_EQ_DEFAULT_SIZE 1000

DIRECT_FN STATIC ssize_t sstmac_eq_read(struct fid_eq *eq, uint32_t *event,
              void *buf, size_t len, uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_readerr(struct fid_eq *eq,
           struct fi_eq_err_entry *buf,
           uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_write(struct fid_eq *eq, uint32_t event,
               const void *buf, size_t len,
               uint64_t flags);

DIRECT_FN STATIC ssize_t sstmac_eq_sread(struct fid_eq *eq, uint32_t *event,
               void *buf, size_t len, int timeout,
               uint64_t flags);

DIRECT_FN STATIC const char *sstmac_eq_strerror(struct fid_eq *eq, int prov_errno,
                const void *err_data, char *buf,
                size_t len);

EXTERN_C DIRECT_FN STATIC  int sstmac_eq_close(struct fid *fid);

EXTERN_C DIRECT_FN STATIC  int sstmac_eq_control(struct fid *eq, int command, void *arg);

static struct fi_ops_eq sstmac_eq_ops = {
  .size = sizeof(struct fi_ops_eq),
  .read = sstmac_eq_read,
  .readerr = sstmac_eq_readerr,
  .write = sstmac_eq_write,
  .sread = sstmac_eq_sread,
  .strerror = sstmac_eq_strerror
};

static struct fi_ops sstmac_fi_eq_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_eq_close,
  .bind = fi_no_bind,
  .control = sstmac_eq_control,
  .ops_open = fi_no_ops_open
};

extern "C" DIRECT_FN  int sstmac_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr, struct fid_eq **eq_ptr, void *context)
{
	if (!fabric)
		return -FI_EINVAL;

  struct sstmac_fid_eq *eq = (sstmac_fid_eq*) calloc(1, sizeof(fid_eq));
  if (!eq)
		return -FI_ENOMEM;

  ErrorDeallocate err(eq, [](void* ptr){
    auto* eq = (sstmac_fid_eq*) ptr;
    free(eq);
  });

  if (!attr)
    return -FI_EINVAL;

  if (!attr->size)
    attr->size = GNIX_EQ_DEFAULT_SIZE;

  // Only support FI_WAIT_SET and FI_WAIT_UNSPEC
  switch (attr->wait_obj) {
  case FI_WAIT_NONE:
    break;
  case FI_WAIT_SET: {
    if (!attr->wait_set) {
      //must be given a wait set!
      return -FI_EINVAL;
    }
    eq->wait = attr->wait_set;
    sstmaci_add_wait(eq->wait, &eq->eq_fid.fid);
    break;
  }
  case FI_WAIT_UNSPEC: {
    struct fi_wait_attr requested = {
      .wait_obj = eq->attr.wait_obj,
      .flags = 0
    };
    sstmac_wait_open(&eq->fabric->fab_fid, &requested, &eq->wait);
    sstmaci_add_wait(eq->wait, &eq->eq_fid.fid);
    break;
  }
  default:
    return -FI_ENOSYS;
  }

  eq->eq_fid.fid.fclass = FI_CLASS_EQ;
  eq->eq_fid.fid.context = context;
  eq->eq_fid.fid.ops = &sstmac_fi_eq_ops;
  eq->eq_fid.ops = &sstmac_eq_ops;
  eq->attr = *attr;

  *eq_ptr = (fid_eq*) &eq->eq_fid;

  return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_eq_close(struct fid *fid)
{
  sstmac_fid_eq* eq = (sstmac_fid_eq*) fid;
  free(eq);
  return FI_SUCCESS;
}


DIRECT_FN STATIC ssize_t sstmac_eq_read(struct fid_eq *eq, uint32_t *event,
              void *buf, size_t len, uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_eq_sread(struct fid_eq *eq, uint32_t *event,
               void *buf, size_t len, int timeout,
               uint64_t flags)
{
  return 0;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_eq_control(struct fid *eq, int command, void *arg)
{
  switch (command) {
  case FI_GETWAIT:
    /* return _sstmac_get_wait_obj(eq_priv->wait, arg); */
    return -FI_ENOSYS;
  default:
    return -FI_EINVAL;
  }
}

DIRECT_FN STATIC ssize_t sstmac_eq_readerr(struct fid_eq *eq,
					 struct fi_eq_err_entry *buf,
					 uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_eq_write(struct fid_eq *eq, uint32_t event,
				       const void *buf, size_t len,
				       uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC const char *sstmac_eq_strerror(struct fid_eq *eq, int prov_errno,
					      const void *err_data, char *buf,
					      size_t len)
{
	return NULL;
}

