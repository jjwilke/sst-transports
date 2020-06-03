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

#include "sstmac.h"

static int __sstmac_mr_refresh(struct sstmac_fid_mem_desc *desc,
	uint64_t addr, uint64_t len);
static int fi_sstmac_mr_close(fid_t fid);
static int fi_sstmac_mr_control(struct fid *fid, int command, void *arg);
DIRECT_FN extern "C" int sstmac_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context);
DIRECT_FN extern "C" int sstmac_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context);
DIRECT_FN extern "C" int sstmac_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr);
DIRECT_FN extern "C" int sstmac_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags);

/* global declarations */
/* memory registration operations */
static struct fi_ops fi_sstmac_mr_ops = {
	.size = sizeof(struct fi_ops),
	.close = fi_sstmac_mr_close,
	.bind = fi_no_bind,
	.control = fi_sstmac_mr_control,
	.ops_open = fi_no_ops_open,
};

DIRECT_FN extern "C" int sstmac_mr_reg(struct fid *fid, const void *buf, size_t len,
	uint64_t access, uint64_t offset,
	uint64_t requested_key, uint64_t flags,
	struct fid_mr **mr, void *context)
{
#if 0
	const struct iovec mr_iov = {
		.iov_base = (void *) buf,
		.iov_len = len,
	};
	const struct fi_mr_attr attr = {
		.mr_iov = &mr_iov,
		.iov_count = 1,
		.access = access,
		.offset = offset,
		.requested_key = requested_key,
		.context = context,
		.auth_key = NULL,
		.auth_key_size = 0,
	};

	return sstmac_mr_regattr(fid, &attr, flags, mr);
#endif
  return FI_SUCCESS;
}

DIRECT_FN extern "C" int sstmac_mr_regv(struct fid *fid, const struct iovec *iov,
	size_t count, uint64_t access,
	uint64_t offset, uint64_t requested_key,
	uint64_t flags, struct fid_mr **mr, void *context)
{
#if 0
	const struct fi_mr_attr attr = {
		.mr_iov = iov,
		.iov_count = count,
		.access = access,
		.offset = offset,
		.requested_key = requested_key,
		.context = context,
		.auth_key = NULL,
		.auth_key_size = 0,
	};

	return sstmac_mr_regattr(fid, &attr, flags, mr);
#endif
  return FI_SUCCESS;
}

DIRECT_FN extern "C" int sstmac_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
	uint64_t flags, struct fid_mr **mr)
{
#if 0
	struct sstmac_fid_domain *domain = container_of(fid,
		struct sstmac_fid_domain, domain_fid.fid);
	struct sstmac_auth_key *auth_key;

	if (!attr)
		return -FI_EINVAL;
	if (!attr->mr_iov || !attr->iov_count)
		return -FI_EINVAL;

	if (domain->mr_iov_limit < attr->iov_count)
		return -FI_EOPNOTSUPP;

	if (FI_VERSION_LT(domain->fabric->fab_fid.api_version,
		FI_VERSION(1, 5)) &&
		(attr->auth_key || attr->auth_key_size))
		return -FI_EINVAL;

	if (attr->auth_key_size) {
		auth_key = SSTMAC_GET_AUTH_KEY(attr->auth_key,
			attr->auth_key_size,
			domain->using_vmdh);
		if (!auth_key)
			return -FI_EINVAL;
	} else {
		auth_key = domain->auth_key;
	}

	if (attr->iov_count == 1)
		return _sstmac_mr_reg(fid, attr->mr_iov[0].iov_base,
			attr->mr_iov[0].iov_len, attr->access, attr->offset,
			attr->requested_key, flags, mr, attr->context,
			auth_key, SSTMAC_USER_REG);

	/* regv limited to one iov at this time */
#endif
	return -FI_EOPNOTSUPP;
}

static int fi_sstmac_mr_control(struct fid *fid, int command, void *arg)
{
	int ret;
#if 0
	struct sstmac_fid_mem_desc *desc;

	desc = container_of(fid, struct sstmac_fid_mem_desc, mr_fid);
	if (desc->mr_fid.fid.fclass != FI_CLASS_MR) {
		SSTMAC_WARN(FI_LOG_DOMAIN, "invalid fid\n");
		return -FI_EINVAL;
	}

	switch (command) {
	case FI_REFRESH:
		ret = __sstmac_mr_refresh_iov(fid, arg);
		break;
	default:
		ret = -FI_EOPNOTSUPP;
		break;
	}
#endif
	return ret;
}

static int fi_sstmac_mr_close(fid_t fid)
{
	sstmac_return_t ret;
#if 0
	struct sstmac_fid_mem_desc *mr;
	struct sstmac_fid_domain *domain;
	struct sstmac_mr_cache_info *info;
	int requested_key;
	struct sstmac_auth_key *auth_key;

	SSTMAC_TRACE(FI_LOG_MR, "\n");

	if (OFI_UNLIKELY(fid->fclass != FI_CLASS_MR))
		return -FI_EINVAL;

	mr = container_of(fid, struct sstmac_fid_mem_desc, mr_fid.fid);

	auth_key = mr->auth_key;
	domain = mr->domain;
	requested_key = fi_mr_key(&mr->mr_fid);
	info = &domain->mr_cache_info[mr->auth_key->ptag];

	/* call cache deregister op */
	fastlock_acquire(&info->mr_cache_lock);
	ret = domain->mr_ops->dereg_mr(domain, mr);
	fastlock_release(&info->mr_cache_lock);

	/* check retcode */
	if (OFI_LIKELY(ret == FI_SUCCESS)) {
		/* release references to the domain and nic */
		_sstmac_ref_put(domain);
		if (auth_key->using_vmdh) {
			if (requested_key < auth_key->attr.user_key_limit)
				_sstmac_test_and_clear_bit(auth_key->user,
					requested_key);
			else {
				ret = _sstmac_release_reserved_key(auth_key,
					requested_key);
				if (ret != FI_SUCCESS) {
					SSTMAC_WARN(FI_LOG_DOMAIN,
						"failed to release reserved key, "
						"rc=%d key=%d\n",
						ret, requested_key);
				}
			}
		}
	} else {
		SSTMAC_INFO(FI_LOG_MR, "failed to deregister memory, "
			  "ret=%i\n", ret);
	}
#endif
	return ret;
}

DIRECT_FN extern "C" int sstmac_mr_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
	return -FI_ENOSYS;
}

