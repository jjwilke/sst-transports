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


#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <assert.h>

#include "sstmac.h"

static int sstmac_stx_close(fid_t fid);
static int sstmac_domain_close(fid_t fid);
DIRECT_FN extern "C" int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags);
static int
sstmac_domain_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context);

DIRECT_FN STATIC extern "C" int sstmac_stx_open(struct fid_domain *dom,
				   struct fi_tx_attr *tx_attr,
				   struct fid_stx **stx, void *context);
DIRECT_FN extern "C" int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags);
DIRECT_FN extern "C" int sstmac_srx_context(struct fid_domain *domain,
			       struct fi_rx_attr *attr,
			       struct fid_ep **rx_ep, void *context);
DIRECT_FN extern "C" int sstmac_domain_open(struct fid_fabric *fabric, struct fi_info *info,
			       struct fid_domain **dom, void *context);

static struct fi_ops sstmac_stx_ops = {
  .close = sstmac_stx_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = fi_no_ops_open
};

static struct fi_ops sstmac_domain_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_domain_close,
  .bind = fi_no_bind,
  .control = fi_no_control,
  .ops_open = sstmac_domain_ops_open
};

static struct fi_ops_mr sstmac_domain_mr_ops = {
  .size = sizeof(struct fi_ops_mr),
  .reg = sstmac_mr_reg,
  .regv = sstmac_mr_regv,
  .regattr = sstmac_mr_regattr,
};

static struct fi_ops_domain sstmac_domain_ops = {
  .size = sizeof(struct fi_ops_domain),
  .av_open = sstmac_av_open,
  .cq_open = sstmac_cq_open,
  .endpoint = sstmac_ep_open,
  .scalable_ep = sstmac_sep_open,
  .cntr_open = sstmac_cntr_open,
  .poll_open = fi_no_poll_open,
  .stx_ctx = sstmac_stx_open,
  .srx_ctx = fi_no_srx_context
};

#define SSTMAC_MR_MODE_DEFAULT FI_MR_BASIC
#define SSTMAC_NUM_PTAGS 256

/*******************************************************************************
 * API function implementations.
 ******************************************************************************/

static int sstmac_stx_close(fid_t fid)
{
#if 0
  struct sstmac_fid_stx *stx;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

  stx = container_of(fid, struct sstmac_fid_stx, stx_fid.fid);
  if (stx->stx_fid.fid.fclass != FI_CLASS_STX_CTX)
    return -FI_EINVAL;

  _sstmac_ref_put(stx->domain);
  _sstmac_ref_put(stx);
#endif
  return FI_SUCCESS;
}

DIRECT_FN STATIC extern "C" int sstmac_stx_open(struct fid_domain *dom,
				   struct fi_tx_attr *tx_attr,
				   struct fid_stx **stx, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_domain *domain;
	struct sstmac_fid_stx *stx_priv;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

	domain = container_of(dom, struct sstmac_fid_domain, domain_fid.fid);
	if (domain->domain_fid.fid.fclass != FI_CLASS_DOMAIN) {
		ret = -FI_EINVAL;
		goto err;
	}

  stx_priv = (sstmac_fid_stx*) calloc(1, sizeof(*stx_priv));
	if (!stx_priv) {
		ret = -FI_ENOMEM;
		goto err;
	}

	stx_priv->domain = domain;
	stx_priv->auth_key = NULL;
	stx_priv->nic = NULL;

	_sstmac_ref_init(&stx_priv->ref_cnt, 1, __stx_destruct);

	_sstmac_ref_get(stx_priv->domain);

	stx_priv->stx_fid.fid.fclass = FI_CLASS_STX_CTX;
	stx_priv->stx_fid.fid.context = context;
  stx_priv->stx_fid.fid.ops = &sstmac_stx_ops;
	stx_priv->stx_fid.ops = NULL;
	domain->num_allocd_stxs++;

	*stx = &stx_priv->stx_fid;
#endif
err:
	return ret;
}



static int sstmac_domain_close(fid_t fid)
{
	int ret = FI_SUCCESS, references_held;
#if 0
	struct sstmac_fid_domain *domain;
	int i;
	struct sstmac_mr_cache_info *info;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

	domain = container_of(fid, struct sstmac_fid_domain, domain_fid.fid);
	if (domain->domain_fid.fid.fclass != FI_CLASS_DOMAIN) {
		ret = -FI_EINVAL;
		goto err;
	}

  for (i = 0; i < SSTMAC_NUM_PTAGS; i++) {
		info = &domain->mr_cache_info[i];

		if (!domain->mr_cache_info[i].inuse)
			continue;

		/* before checking the refcnt,
		 * flush the memory registration cache
		 */
		if (info->mr_cache_ro) {
			fastlock_acquire(&info->mr_cache_lock);
			ret = _sstmac_mr_cache_flush(info->mr_cache_ro);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_DOMAIN,
					  "failed to flush memory cache on domain close\n");
				fastlock_release(&info->mr_cache_lock);
				goto err;
			}
			fastlock_release(&info->mr_cache_lock);
		}

		if (info->mr_cache_rw) {
			fastlock_acquire(&info->mr_cache_lock);
			ret = _sstmac_mr_cache_flush(info->mr_cache_rw);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_DOMAIN,
					  "failed to flush memory cache on domain close\n");
				fastlock_release(&info->mr_cache_lock);
				goto err;
			}
			fastlock_release(&info->mr_cache_lock);
		}
	}

	/*
	 * if non-zero refcnt, there are eps, mrs, and/or an eq associated
	 * with this domain which have not been closed.
	 */

	references_held = _sstmac_ref_put(domain);

	if (references_held) {
    SSTMAC_INFO(FI_LOG_DOMAIN, "failed to fully close domain due to "
			  "lingering references. references=%i dom=%p\n",
			  references_held, domain);
	}

  SSTMAC_INFO(FI_LOG_DOMAIN, "sstmac_domain_close invoked returning %d\n",
		  ret);
#endif
err:
	return ret;
}

/*
 * sstmac_domain_ops provides a means for an application to better
 * control allocation of underlying aries resources associated with
 * the domain.  Examples will include controlling size of underlying
 * hardware CQ sizes, max size of RX ring buffers, etc.
 */

static const uint32_t default_msg_rendezvous_thresh = 16*1024;
static const uint32_t default_rma_rdma_thresh = 8*1024;
static const uint32_t default_ct_init_size = 64;
static const uint32_t default_ct_max_size = 16384;
static const uint32_t default_ct_step = 2;
static const uint32_t default_vc_id_table_capacity = 128;
//static const uint32_t default_mbox_page_size = SSTMAC_PAGE_2MB;
static const uint32_t default_mbox_num_per_slab = 2048;
static const uint32_t default_mbox_maxcredit = 64;
static const uint32_t default_mbox_msg_maxsize = 16384;
/* rx cq bigger to avoid having to deal with rx overruns so much */
static const uint32_t default_rx_cq_size = 16384;
static const uint32_t default_tx_cq_size = 2048;
static const uint32_t default_max_retransmits = 5;
static const int32_t default_err_inject_count = 0;
static const uint32_t default_dgram_progress_timeout = 100;
static const uint32_t default_eager_auto_progress = 0;

DIRECT_FN extern "C" int sstmac_domain_bind(struct fid_domain *domain, struct fid *fid,
			       uint64_t flags)
{
	return -FI_ENOSYS;
}

static int
sstmac_domain_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	if (strcmp(ops_name, FI_SSTMAC_DOMAIN_OPS_1) == 0)
		*ops = &sstmac_ops_domain;
	else
		ret = -FI_EINVAL;
#endif
	return ret;
}



DIRECT_FN extern "C" int sstmac_domain_open(struct fid_fabric *fabric, struct fi_info *info,
			       struct fid_domain **dom, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_domain *domain = NULL;
	struct sstmac_fid_fabric *fabric_priv;
	struct sstmac_auth_key *auth_key = NULL;
	int i;
	int requesting_vmdh = 0;

  SSTMAC_TRACE(FI_LOG_DOMAIN, "\n");

	fabric_priv = container_of(fabric, struct sstmac_fid_fabric, fab_fid);

	if (FI_VERSION_LT(fabric->api_version, FI_VERSION(1, 5)) &&
		(info->domain_attr->auth_key_size || info->domain_attr->auth_key))
			return -FI_EINVAL;

	requesting_vmdh = !(info->domain_attr->mr_mode &
			(FI_MR_BASIC | FI_MR_VIRT_ADDR));

  auth_key = SSTMAC_GET_AUTH_KEY(info->domain_attr->auth_key,
			info->domain_attr->auth_key_size, requesting_vmdh);
	if (!auth_key)
		return -FI_EINVAL;

  SSTMAC_INFO(FI_LOG_DOMAIN,
		  "authorization key=%p ptag %u cookie 0x%x\n",
		  auth_key, auth_key->ptag, auth_key->cookie);

	if (auth_key->using_vmdh != requesting_vmdh) {
    SSTMAC_WARN(FI_LOG_DOMAIN,
      "SSTMAC provider cannot support multiple "
			"FI_MR_BASIC and FI_MR_SCALABLE for the same ptag. "
			"ptag=%d current_mode=%x requested_mode=%x\n",
			auth_key->ptag,
			auth_key->using_vmdh, info->domain_attr->mr_mode);
		return -FI_EINVAL;
	}

  domain = (sstmac_fid_domain*) calloc(1, sizeof *domain);
	if (domain == NULL) {
		ret = -FI_ENOMEM;
		goto err;
	}

  domain->mr_cache_info = (sstmac_mr_cache_info*) calloc(sizeof(*domain->mr_cache_info),
    SSTMAC_NUM_PTAGS);
	if (!domain->mr_cache_info) {
		ret = -FI_ENOMEM;
		goto err;
	}

	domain->auth_key = auth_key;

	domain->mr_cache_attr = _sstmac_default_mr_cache_attr;
	domain->mr_cache_attr.reg_context = (void *) domain;
	domain->mr_cache_attr.dereg_context = NULL;
	domain->mr_cache_attr.destruct_context = NULL;

	ret = _sstmac_smrn_open(&domain->mr_cache_attr.smrn);
	if (ret != FI_SUCCESS)
		goto err;

	fastlock_init(&domain->mr_cache_lock);
  for (i = 0; i < SSTMAC_NUM_PTAGS; i++) {
		domain->mr_cache_info[i].inuse = 0;
		domain->mr_cache_info[i].domain = domain;
		fastlock_init(&domain->mr_cache_info[i].mr_cache_lock);
	}

	/*
	 * we are likely sharing udreg entries with Craypich if we're using udreg
	 * cache, so ask for only half the entries by default.
	 */
	domain->udreg_reg_limit = 2048;

	dlist_init(&domain->nic_list);
	dlist_init(&domain->list);

	dlist_insert_after(&domain->list, &fabric_priv->domain_list);

	domain->fabric = fabric_priv;
	_sstmac_ref_get(domain->fabric);

	domain->cdm_id_seed = getpid();  /* TODO: direct syscall better */
	domain->addr_format = info->addr_format;

	/* user tunables */
	domain->params.msg_rendezvous_thresh = default_msg_rendezvous_thresh;
	domain->params.rma_rdma_thresh = default_rma_rdma_thresh;
	domain->params.ct_init_size = default_ct_init_size;
	domain->params.ct_max_size = default_ct_max_size;
	domain->params.ct_step = default_ct_step;
	domain->params.vc_id_table_capacity = default_vc_id_table_capacity;
	domain->params.mbox_page_size = default_mbox_page_size;
	domain->params.mbox_num_per_slab = default_mbox_num_per_slab;
	domain->params.mbox_maxcredit = default_mbox_maxcredit;
	domain->params.mbox_msg_maxsize = default_mbox_msg_maxsize;
	domain->params.rx_cq_size = default_rx_cq_size;
	domain->params.tx_cq_size = default_tx_cq_size;
	domain->params.max_retransmits = default_max_retransmits;
	domain->params.err_inject_count = default_err_inject_count;
#if HAVE_XPMEM
	domain->params.xpmem_enabled = true;
#else
	domain->params.xpmem_enabled = false;
#endif
	domain->params.dgram_progress_timeout = default_dgram_progress_timeout;
	domain->params.eager_auto_progress = default_eager_auto_progress;

	domain->sstmac_cq_modes = sstmac_def_sstmac_cq_modes;
	_sstmac_ref_init(&domain->ref_cnt, 1, __domain_destruct);

	domain->domain_fid.fid.fclass = FI_CLASS_DOMAIN;
	domain->domain_fid.fid.context = context;
	domain->domain_fid.fid.ops = &sstmac_domain_fi_ops;
	domain->domain_fid.ops = &sstmac_domain_ops;
	domain->domain_fid.mr = &sstmac_domain_mr_ops;

	domain->control_progress = info->domain_attr->control_progress;
	domain->data_progress = info->domain_attr->data_progress;
	domain->thread_model = info->domain_attr->threading;
	domain->mr_is_init = 0;
	domain->mr_iov_limit = info->domain_attr->mr_iov_limit;

	fastlock_init(&domain->cm_nic_lock);

	domain->using_vmdh = requesting_vmdh;

	auth_key->using_vmdh = domain->using_vmdh;
	_sstmac_auth_key_enable(auth_key);
	domain->auth_key = auth_key;

	if (!requesting_vmdh) {
    _sstmac_open_cache(domain, SSTMAC_DEFAULT_CACHE_TYPE);
	} else {
    domain->mr_cache_type = SSTMAC_MR_TYPE_NONE;
    _sstmac_open_cache(domain, SSTMAC_MR_TYPE_NONE);
	}

	*dom = &domain->domain_fid;
	return FI_SUCCESS;

err:
	if (domain && domain->mr_cache_info)
		free(domain->mr_cache_info);

	if (domain != NULL) {
		free(domain);
	}
#endif
	return ret;
}

DIRECT_FN extern "C" int sstmac_srx_context(struct fid_domain *domain,
			       struct fi_rx_attr *attr,
			       struct fid_ep **rx_ep, void *context)
{
	return -FI_ENOSYS;
}


