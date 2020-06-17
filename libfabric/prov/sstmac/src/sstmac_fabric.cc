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

static void sstmac_fini(void)
{
}

#define SSTMAC_EP_CAPS   \
  FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS | \
  FI_DIRECTED_RECV | FI_READ | FI_NAMED_RX_CTX | \
  FI_WRITE | FI_SEND | FI_RECV | FI_REMOTE_READ | FI_REMOTE_WRITE)

#define SSTMAC_DOM_CAPS \
  FI_LOCAL_COMM | FI_REMOTE_COMM | FI_SHARED_AV

#define SSTMAC_MAX_MSG_SIZE 1<<62


static struct fi_info *sstmac_allocinfo(void)
{
  struct fi_info *sstmac_info;

  sstmac_info = fi_allocinfo();
  if (sstmac_info == NULL) {
    return NULL;
  }

  sstmac_info->caps = SSTMAC_EP_PRIMARY_CAPS;
  sstmac_info->tx_attr->op_flags = 0;
  sstmac_info->rx_attr->op_flags = 0;
  sstmac_info->ep_attr->type = FI_EP_RDM;
  sstmac_info->ep_attr->protocol = FI_PROTO_SSTMAC;
  sstmac_info->ep_attr->max_msg_size = SSTMAC_MAX_MSG_SIZE;
  sstmac_info->ep_attr->mem_tag_format = FI_TAG_GENERIC;
  sstmac_info->ep_attr->tx_ctx_cnt = 1;
  sstmac_info->ep_attr->rx_ctx_cnt = 1;


  sstmac_info->domain_attr->name = strdup("sstmac");
  sstmac_info->domain_attr->threading = FI_THREAD_SAFE;
  sstmac_info->domain_attr->control_progress = FI_PROGRESS_AUTO;
  sstmac_info->domain_attr->data_progress = FI_PROGRESS_AUTO;
  sstmac_info->domain_attr->av_type = FI_AV_TABLE;

  //says the applicatoin is protected from buffer overruns on things like CQs
  //this is always true in SSTMAC since we push back on dynamic bufers
  sstmac_info->domain_attr->resource_mgmt = FI_RM_ENABLED;

  //compatibility enum, basically just says that you need to request via full virtual addr,
  //that memory regions must be allocated (backed by phsical pages), and that keys are allocated by provider
  sstmac_info->domain_attr->mr_mode = FI_MR_BASIC;

  //the size of the mr_key, just make it 64-bit for now
  sstmac_info->domain_attr->mr_key_size = sizeof(uint64_t);
  //the size of the data written to completion queues as part of an event, make 64-bit for now
  sstmac_info->domain_attr->cq_data_size = sizeof(uint64_t);
  //just set to a big number - the maximum number of completion queues
  sstmac_info->domain_attr->cq_cnt = 1000;
  //set to largest possible 32-bit number - the maxmimum number of endpoints
  sstmac_info->domain_attr->ep_cnt = std::numeric_limits<uint32_t>::max();
  //just set to a big number - the maximnum number of tx contexts that can be handled
  sstmac_info->domain_attr->tx_ctx_cnt = 1000;
  //just set to a big number - the maximum number of tx contexts that can be handled
  sstmac_info->domain_attr->rx_ctx_cnt = 1000;
  //just set to a big number
  sstmac_info->domain_attr->max_ep_tx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_rx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_stx_ctx = 1000;
  sstmac_info->domain_attr->max_ep_srx_ctx = 1000;
  //just set to a big number - the maximum number of different completion counters
  sstmac_info->domain_attr->cntr_cnt = 1000;
  //we don't really do anything with mr_iov, so allow a big number
  sstmac_info->domain_attr->mr_iov_limit = 10000;
  //we support everything - send local, send remote, share av tables
  sstmac_info->domain_attr->caps = SSTMAC_DOM_CAPS;
  //we place no restrictions on domains having the exact same capabilities to communicate
  sstmac_info->domain_attr->mode = 0;
  //we don't really do anything with keys, so...
  sstmac_info->domain_attr->auth_key = 0;
  sstmac_info->domain_attr->max_err_data = sizeof(uint64_t);
  //we have no limit on the number of memory regions
  sstmac_info->domain_attr->mr_cnt = std::numeric_limits<uint32_t>::max();



  sstmac_info->next = NULL;
  sstmac_info->addr_format = FI_ADDR_GNI;
  sstmac_info->src_addrlen = sizeof(struct sstmac_ep_name);
  sstmac_info->dest_addrlen = sizeof(struct sstmac_ep_name);
  sstmac_info->src_addr = NULL;
  sstmac_info->dest_addr = NULL;

#if 0
  sstmac_info->tx_attr->msg_order = FI_ORDER_SAS;
  sstmac_info->tx_attr->comp_order = FI_ORDER_NONE;
  sstmac_info->tx_attr->size = GNIX_TX_SIZE_DEFAULT;
  sstmac_info->tx_attr->iov_limit = GNIX_MAX_MSG_IOV_LIMIT;
  sstmac_info->tx_attr->inject_size = GNIX_INJECT_SIZE;
  sstmac_info->tx_attr->rma_iov_limit = GNIX_MAX_RMA_IOV_LIMIT;
  sstmac_info->rx_attr->msg_order = FI_ORDER_SAS;
  sstmac_info->rx_attr->comp_order = FI_ORDER_NONE;
  sstmac_info->rx_attr->size = GNIX_RX_SIZE_DEFAULT;
  sstmac_info->rx_attr->iov_limit = GNIX_MAX_MSG_IOV_LIMIT;
#endif
  return sstmac_info;
}


static int sstmac_getinfo(uint32_t version, const char *node, const char *service,
      uint64_t flags, const struct fi_info *hints,
      struct fi_info **info_ptr)
{
  fi_info* info = new fi_info;
  uint64_t init_caps = hints ? hints->caps : 0;
  info->caps = init_caps & (
      FI_ATOMIC
      // & FI_DIRECTED_RECV TOOD
      // & FI_FENCE TODO
      | FI_HMEM
      | FI_LOCAL_COMM
      | FI_MSG
      // & FI_MULTICAST TODO
      // & FI_MULTI_RECV TODO
      // & FI_NAMED_RX_CTX TODO
      | FI_READ
      | FI_RECV
      | FI_REMOTE_COMM
      | FI_REMOTE_READ
      | FI_REMOTE_WRITE
      | FI_RMA
      | FI_RMA_EVENT
      // & FI_RMA_MEM TODO
      | FI_SEND
      | FI_SHARED_AV
      // & FI_SOURCE TODO
      // & FI_SOURCE_ERR TODO
      | FI_TAGGED
      | FI_TRIGGER
      // & FI_VARIABLE_MSG TODO
      | FI_WRITE
  );
  info->mode = 0;
  if (hints->mode){
    uint64_t clear = FI_ASYNC_IOV | FI_MSG_PREFIX | FI_RX_CQ_DATA;
    //info->mode = hints->mode & ~clear;
  }
  return 0;
}

struct fi_provider sstmac_prov = {
  .name = "sstmac",
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
