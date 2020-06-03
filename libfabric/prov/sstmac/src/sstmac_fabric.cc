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
/*
 * Copyright (c) 2014 Intel Corporation, Inc.  All rights reserved.
 * Copyright (c) 2015-2017 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2015-2017 Cray Inc. All rights reserved.
 * Copyrigth (c) 2019      Triad National Security, LLC. All rights
 *                         reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <ofi_util.h>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_errno.h>
#include "ofi_prov.h"

#include "sstmac.h"
#include "sstmac_wait.h"

/* check if only one bit of a set is enabled, when one is required */
#define IS_EXCLUSIVE(x) \
	((x) && !((x) & ((x)-1)))

/* optional basic bits */
#define SSTMAC_MR_BASIC_OPT \
	(FI_MR_LOCAL)

/* optional scalable bits */
#define SSTMAC_MR_SCALABLE_OPT \
	(FI_MR_LOCAL)

/* required basic bits */
#define SSTMAC_MR_BASIC_REQ \
	(FI_MR_VIRT_ADDR | FI_MR_ALLOCATED | FI_MR_PROV_KEY)

/* required scalable bits */
#define SSTMAC_MR_SCALABLE_REQ \
	(FI_MR_MMU_NOTIFY)

#define SSTMAC_MR_BASIC_BITS \
	(SSTMAC_MR_BASIC_OPT | SSTMAC_MR_BASIC_REQ)

#define SSTMAC_MR_SCALABLE_BITS \
	(SSTMAC_MR_SCALABLE_OPT | SSTMAC_MR_SCALABLE_REQ)

#define SSTMAC_DEFAULT_USER_REGISTRATION_LIMIT 192
#define SSTMAC_DEFAULT_PROV_REGISTRATION_LIMIT 64
#define SSTMAC_DEFAULT_SHARED_MEMORY_TIMEOUT 30

extern "C" int sstmac_default_user_registration_limit = SSTMAC_DEFAULT_USER_REGISTRATION_LIMIT;
extern "C" int sstmac_default_prov_registration_limit = SSTMAC_DEFAULT_PROV_REGISTRATION_LIMIT;
uint32_t sstmac_wait_shared_memory_timeout = SSTMAC_DEFAULT_SHARED_MEMORY_TIMEOUT;

/* assume that the user will open additional fabrics later and that
   ptag information will need to be retained for the lifetime of the
   process. If the user sets this value, we can assume that they
   intend to be done with libfabric when the last fabric instance
   closes so that we can free the ptag information. */
extern "C" int sstmac_dealloc_aki_on_fabric_close = 0;

#define SSTMAC_MAJOR_VERSION 1
#define SSTMAC_MINOR_VERSION 0

const struct fi_fabric_attr sstmac_fabric_attr = {
	.fabric = NULL,
	.name = NULL,
	.prov_name = NULL,
	.prov_version = FI_VERSION(SSTMAC_MAJOR_VERSION, SSTMAC_MINOR_VERSION),
};

DIRECT_FN extern "C" int sstmac_fabric_trywait(struct fid_fabric *fabric, struct fid **fids, int count)
{
	return -FI_ENOSYS;
}

static int sstmac_fabric_close(fid_t fid);
static int sstmac_fab_ops_open(struct fid *fid, const char *ops_name,
        uint64_t flags, void **ops, void *context);

static struct fi_ops sstmac_fab_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_fabric_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = sstmac_fab_ops_open,
};

static struct fi_ops_fabric sstmac_fab_ops = {
  .size = sizeof(struct fi_ops_fabric),
  .domain = sstmac_domain_open,
  .passive_ep = sstmac_pep_open,
  .eq_open = sstmac_eq_open,
  .wait_open = sstmac_wait_open,
  .trywait = sstmac_fabric_trywait
};

static int sstmac_fabric_close(fid_t fid)
{
  //TODO
	return FI_SUCCESS;
}

/*
 * define methods needed for the SSTMAC fabric provider
 */
static int sstmac_fabric_open(struct fi_fabric_attr *attr,
			    struct fid_fabric **fabric,
			    void *context)
{
  //TODO
	return FI_SUCCESS;
}

const char sstmac_fab_name[] = "sstmac";
const char sstmac_dom_name[] = "sstmac";
const char sstmac_prov_name[] = "sstmac";

static void sstmac_fini(void)
{
}


static int sstmac_getinfo(uint32_t version, const char *node, const char *service,
      uint64_t flags, const struct fi_info *hints,
      struct fi_info **info)
{
  //TODO
  return 0;
}

struct fi_provider sstmac_prov = {
	.name = sstmac_prov_name,
	.version = FI_VERSION(SSTMAC_MAJOR_VERSION, SSTMAC_MINOR_VERSION),
	.fi_version = OFI_VERSION_LATEST,
	.getinfo = sstmac_getinfo,
	.fabric = sstmac_fabric_open,
	.cleanup = sstmac_fini
};

__attribute__((visibility ("default"),EXTERNALLY_VISIBLE)) \
struct fi_provider* fi_prov_ini(void)
{
	struct fi_provider *provider = NULL;
	sstmac_return_t status;
  //sstmac_version_info_t lib_version;
	int num_devices;
	int ret;
	return (provider);
}
