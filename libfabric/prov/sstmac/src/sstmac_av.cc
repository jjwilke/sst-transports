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
 * Copyright (c) 2015-2017 Cray Inc. All rights reserved.
 * Copyright (c) 2015-2017 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2019      Triad National Security, LLC.
 *                         All rights reserved.
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

//
// Address vector common code
//
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sstmac.h"
#include "sstmac_av.h"

/*
 * local variables and structs
 */

#define SSTMAC_AV_ENTRY_VALID		(1ULL)
#define SSTMAC_AV_ENTRY_CM_NIC_ID		(1ULL << 2)

struct sstmac_av_block {
	struct slist_entry        slist;
  struct sstmac_av_addr_entry *base;
};

DIRECT_FN STATIC extern "C" int sstmac_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
            uint64_t flags, void *context);

DIRECT_FN STATIC extern "C" int sstmac_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

DIRECT_FN STATIC extern "C" int sstmac_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context);

DIRECT_FN STATIC extern "C" int sstmac_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags);

DIRECT_FN STATIC extern "C" int sstmac_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen);

DIRECT_FN const char *sstmac_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len);

static int sstmac_av_close(fid_t fid);

/*******************************************************************************
 * FI_OPS_* data structures.
 ******************************************************************************/
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

/*
 * Note: this function (according to WG), is not intended to
 * typically be used in the critical path for messaging/rma/amo
 * requests
 */
DIRECT_FN STATIC extern "C" int sstmac_av_lookup(struct fid_av *av, fi_addr_t fi_addr,
				    void *addr, size_t *addrlen)
{
	int ret;
#if 0
  struct sstmac_fid_av *sstmac_av;
  struct sstmac_ep_name ep_name = { {0} };
  struct sstmac_av_addr_entry entry;

  SSTMAC_TRACE(FI_LOG_AV, "\n");

	if (!av || !addrlen)
		return -FI_EINVAL;

  sstmac_av = container_of(av, struct sstmac_fid_av, av_fid);

  if (sstmac_av->domain->addr_format == FI_ADDR_STR) {
    if (*addrlen < SSTMAC_FI_ADDR_STR_LEN) {
      *addrlen = SSTMAC_FI_ADDR_STR_LEN;
			return -FI_ETOOSMALL;
		}
	} else {
		if (*addrlen < sizeof(ep_name)) {
			*addrlen = sizeof(ep_name);
			return -FI_ETOOSMALL;
		}
	}

	/*
	 * user better have provided a buffer since the
	 * value stored in addrlen is big enough to return ep_name
	 */

	if (!addr)
		return -FI_EINVAL;

  ret = _sstmac_av_lookup(sstmac_av, fi_addr, &entry);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_AV, "_sstmac_av_lookup failed: %d\n", ret);
		return ret;
	}

  ep_name.sstmac_addr = entry.sstmac_addr;
	ep_name.name_type = entry.name_type;
	ep_name.cm_nic_cdm_id = entry.cm_nic_cdm_id;
	ep_name.cookie = entry.cookie;

  if (sstmac_av->domain->addr_format == FI_ADDR_STR) {
    ret = _sstmac_ep_name_to_str(&ep_name, (char **)&addr);
		if (ret != FI_SUCCESS) {
      SSTMAC_WARN(FI_LOG_AV, "_sstmac_resolve_str_ep_name failed: %d %s\n",
				  ret, fi_strerror(-ret));
			return ret;
		}
    *addrlen = SSTMAC_FI_ADDR_STR_LEN;
	} else {
		memcpy(addr, (void *)&ep_name, MIN(*addrlen, sizeof(ep_name)));
		*addrlen = sizeof(ep_name);
	}
#endif
	return FI_SUCCESS;
}

DIRECT_FN STATIC extern "C" int sstmac_av_insert(struct fid_av *av, const void *addr,
				    size_t count, fi_addr_t *fi_addr,
				    uint64_t flags, void *context)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_av *av_priv = NULL;

  SSTMAC_TRACE(FI_LOG_AV, "\n");

	if (!av)
		return -FI_EINVAL;

  av_priv = container_of(av, struct sstmac_fid_av, av_fid);

	if (!av_priv)
		return -FI_EINVAL;

	if ((av_priv->type == FI_AV_MAP) && (fi_addr == NULL))
		return -FI_EINVAL;

	if ((flags & FI_SYNC_ERR) && (context == NULL)) {
    SSTMAC_WARN(FI_LOG_AV, "FI_SYNC_ERR requires context\n");
		return -FI_EINVAL;
	}

	switch (av_priv->type) {
	case FI_AV_TABLE:
		ret =
		    table_insert(av_priv, addr, count, fi_addr, flags, context);
		break;
	case FI_AV_MAP:
		ret = map_insert(av_priv, addr, count, fi_addr, flags, context);
		break;
	default:
		ret = -FI_EINVAL;
		break;
	}

#endif
	return ret;
}

DIRECT_FN STATIC extern "C" int sstmac_av_insertsvc(struct fid_av *av, const char *node,
				       const char *service, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

DIRECT_FN STATIC extern "C" int sstmac_av_insertsym(struct fid_av *av, const char *node,
				       size_t nodecnt, const char *service,
				       size_t svccnt, fi_addr_t *fi_addr,
				       uint64_t flags, void *context)
{
	return -FI_ENOSYS;
}

DIRECT_FN STATIC extern "C" int sstmac_av_remove(struct fid_av *av, fi_addr_t *fi_addr,
				    size_t count, uint64_t flags)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_av *av_priv = NULL;

  SSTMAC_TRACE(FI_LOG_AV, "\n");

	if (!av) {
		ret = -FI_EINVAL;
		goto err;
	}

  av_priv = container_of(av, struct sstmac_fid_av, av_fid);

	if (!av_priv) {
		ret = -FI_EINVAL;
		goto err;
	}

	switch (av_priv->type) {
	case FI_AV_TABLE:
		ret = table_remove(av_priv, fi_addr, count, flags);
		break;
	case FI_AV_MAP:
		ret = map_remove(av_priv, fi_addr, count, flags);
		break;
	default:
		ret = -FI_EINVAL;
		break;
	}
#endif
err:
	return ret;
}

/*
 * Given an address pointed to by addr, stuff a string into buf representing:
 * device_addr:cdm_id:name_type:cm_nic_cdm_id:cookie
 * where device_addr, cdm_id, cm_nic_cdm_id and cookie are represented in
 * hexadecimal. And name_type is represented as an integer.
 */
DIRECT_FN const char *sstmac_av_straddr(struct fid_av *av,
		const void *addr, char *buf,
		size_t *len)
{
#if 0
  char int_buf[SSTMAC_AV_MAX_STR_ADDR_LEN];
	int size;
  struct sstmac_ep_name ep_name;
  struct sstmac_fid_av *av_priv;

	if (!av || !addr || !buf || !len) {
    SSTMAC_DEBUG(FI_LOG_DEBUG, "NULL parameter in sstmac_av_straddr\n");
		return NULL;
	}

  av_priv = container_of(av, struct sstmac_fid_av, av_fid);

	if (av_priv->domain->addr_format == FI_ADDR_STR)
    _sstmac_resolve_str_ep_name((const char*)addr, 0, &ep_name);
	else
    ep_name = ((struct sstmac_ep_name *) addr)[0];

	/*
	 * if additional information is added to this string, then
	 * you will need to update in sstmacx.h:
   *   SSTMAC_AV_STR_ADDR_VERSION, increment this value
   *   SSTMAC_AV_MAX_STR_ADDR_LEN, to be the number of characters printed
	 */
	size = snprintf(int_buf, sizeof(int_buf), "%04i:0x%08" PRIx32 ":0x%08"
			PRIx32 ":%02i:0x%06" PRIx32 ":0x%08" PRIx32
      ":%02i", SSTMAC_AV_STR_ADDR_VERSION,
      ep_name.sstmac_addr.device_addr,
      ep_name.sstmac_addr.cdm_id,
			ep_name.name_type,
			ep_name.cm_nic_cdm_id,
			ep_name.cookie,
			ep_name.rx_ctx_cnt);

	/*
	 * snprintf returns the number of character written
	 * without the terminating character.
	 */
	if ((size + 1) < *len) {
		/*
		 * size needs to be all the characters plus the terminating
		 * character.  Otherwise, we could lose information.
		 */
		size = size + 1;
	} else {
		/* Do not overwrite the buffer. */
		size = *len;
	}

	snprintf(buf, size, "%s", int_buf);
	*len = size;
#endif
	return buf;
}

static int sstmac_av_close(fid_t fid)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_av *av = NULL;
	int references_held;

  SSTMAC_TRACE(FI_LOG_AV, "\n");

	if (!fid) {
		ret = -FI_EINVAL;
		goto err;
	}
  av = container_of(fid, struct sstmac_fid_av, av_fid.fid);

  references_held = _sstmac_ref_put(av);
	if (references_held) {
    SSTMAC_INFO(FI_LOG_AV, "failed to fully close av due to lingering "
				"references. references=%i av=%p\n",
				references_held, av);
	}
#endif

err:
	return ret;
}



DIRECT_FN extern "C" int sstmac_av_bind(struct fid_av *av, struct fid *fid, uint64_t flags)
{
	return -FI_ENOSYS;
}

/*
 * TODO: Support shared named AVs.
 */
DIRECT_FN extern "C" int sstmac_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
			   struct fid_av **av, void *context)
{
	int ret = FI_SUCCESS;
#if 0
  struct sstmac_fid_domain *int_dom = NULL;
  struct sstmac_fid_av *av_priv = NULL;
  struct sstmac_hashtable_attr ht_attr;

	enum fi_av_type type = FI_AV_TABLE;
	size_t count = 128;
	int rx_ctx_bits = 0;

  SSTMAC_TRACE(FI_LOG_AV, "\n");

	if (!domain) {
		ret = -FI_EINVAL;
		goto err;
	}

  int_dom = container_of(domain, struct sstmac_fid_domain, domain_fid);
	if (!int_dom) {
		ret = -FI_EINVAL;
		goto err;
	}

  av_priv = (sstmac_fid_av *)calloc(1, sizeof(*av_priv));
	if (!av_priv) {
		ret = -FI_ENOMEM;
		goto err;
	}

	if (attr) {
    ret = sstmac_verify_av_attr(attr);
		if (ret) {
			goto cleanup;
		}

		if (attr->type != FI_AV_UNSPEC) {
			type = attr->type;
		}
		count = attr->count;
		rx_ctx_bits = attr->rx_ctx_bits;
	}

	av_priv->domain = int_dom;
	av_priv->type = type;
  av_priv->addrlen = sizeof(struct sstmac_address);
	av_priv->rx_ctx_bits = rx_ctx_bits;
	av_priv->mask = rx_ctx_bits ?
			((uint64_t)1 << (64 - attr->rx_ctx_bits)) - 1 : ~0;

	av_priv->capacity = count;
	if (type == FI_AV_TABLE) {
    av_priv->table = (sstmac_av_addr_entry*) calloc(count,
               sizeof(struct sstmac_av_addr_entry));
		if (!av_priv->table) {
			ret = -FI_ENOMEM;
			goto cleanup;
		}
	}

  av_priv->valid_entry_vec = (int*) calloc(count, sizeof(int));
	if (!av_priv->valid_entry_vec) {
		ret = -FI_ENOMEM;
		goto cleanup;
	}

	av_priv->av_fid.fid.fclass = FI_CLASS_AV;
	av_priv->av_fid.fid.context = context;
  av_priv->av_fid.fid.ops = &sstmac_fi_av_ops;
  av_priv->av_fid.ops = &sstmac_av_ops;

	if (type == FI_AV_MAP) {
    av_priv->map_ht = (sstmac_hashtable*) calloc(1, sizeof(struct sstmac_hashtable));
		if (av_priv->map_ht == NULL)
			goto cleanup;

		/*
		 * use same parameters as used for ep vc hash
		 */

		ht_attr.ht_initial_size = int_dom->params.ct_init_size;
		ht_attr.ht_maximum_size = int_dom->params.ct_max_size;
		ht_attr.ht_increase_step = int_dom->params.ct_step;
    ht_attr.ht_increase_type = SSTMAC_HT_INCREASE_MULT;
		ht_attr.ht_collision_thresh = 500;
		ht_attr.ht_hash_seed = 0xdeadbeefbeefdead;
		ht_attr.ht_internal_locking = 1;
		ht_attr.destructor = NULL;

    ret = _sstmac_ht_init(av_priv->map_ht,
				    &ht_attr);
		slist_init(&av_priv->block_list);
	}
  _sstmac_ref_init(&av_priv->ref_cnt, 1, __av_destruct);

	*av = &av_priv->av_fid;

	return ret;

cleanup:
	if (av_priv->table != NULL)
		free(av_priv->table);
	if (av_priv->valid_entry_vec != NULL)
		free(av_priv->valid_entry_vec);
	free(av_priv);
#endif
err:
	return ret;
}

