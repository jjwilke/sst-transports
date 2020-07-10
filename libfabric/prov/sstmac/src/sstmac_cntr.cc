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

#include "sstmac.h"

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_wait(struct fid_cntr *cntr, uint64_t threshold,
				    int timeout);
EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_adderr(struct fid_cntr *cntr, uint64_t value);
EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_seterr(struct fid_cntr *cntr, uint64_t value);
DIRECT_FN STATIC uint64_t sstmac_cntr_readerr(struct fid_cntr *cntr);
EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_seterr(struct fid_cntr *cntr, uint64_t value);
DIRECT_FN STATIC uint64_t sstmac_cntr_read(struct fid_cntr *cntr);
EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_add(struct fid_cntr *cntr, uint64_t value);
extern "C" DIRECT_FN  int sstmac_cntr_open(struct fid_domain *domain,
			     struct fi_cntr_attr *attr,
			     struct fid_cntr **cntr, void *context);
static int sstmac_cntr_control(struct fid *cntr, int command, void *arg);
static int sstmac_cntr_close(fid_t fid);
EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_set(struct fid_cntr *cntr, uint64_t value);

static struct fi_ops sstmac_cntr_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_cntr_close,
  .bind = fi_no_bind,
  .control = sstmac_cntr_control,
  .ops_open = fi_no_ops_open,
};

static struct fi_ops_cntr sstmac_cntr_ops = {
  .size = sizeof(struct fi_ops_cntr),
  .read = sstmac_cntr_read,
  .readerr = sstmac_cntr_readerr,
  .add = sstmac_cntr_add,
  .set = sstmac_cntr_set,
  .wait = sstmac_cntr_wait,
  .adderr = sstmac_cntr_adderr,
  .seterr = sstmac_cntr_seterr
};

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_wait(struct fid_cntr *cntr, uint64_t threshold,
				    int timeout)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_adderr(struct fid_cntr *cntr, uint64_t value)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_seterr(struct fid_cntr *cntr, uint64_t value)
{
  return -FI_ENOSYS;
}

static int sstmac_cntr_close(fid_t fid)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC uint64_t sstmac_cntr_readerr(struct fid_cntr *cntr)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC uint64_t sstmac_cntr_read(struct fid_cntr *cntr)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_add(struct fid_cntr *cntr, uint64_t value)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_cntr_set(struct fid_cntr *cntr, uint64_t value)
{
  return -FI_ENOSYS;
}

static int sstmac_cntr_control(struct fid *cntr, int command, void *arg)
{
  return -FI_ENOSYS;
}

extern "C" DIRECT_FN  int sstmac_cntr_open(struct fid_domain *domain,
			     struct fi_cntr_attr *attr,
			     struct fid_cntr **cntr, void *context)
{
  return -FI_ENOSYS;
}

