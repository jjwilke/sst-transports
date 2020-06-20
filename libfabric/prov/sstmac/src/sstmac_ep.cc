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
 * Copyright (c) 2015-2019 Cray Inc. All rights reserved.
 * Copyright (c) 2015-2018 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2019 Triad National Security, LLC. All rights reserved.
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

/*
 * Endpoint common code
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sstmac.h"
#include "sstmac_ep.h"

#include <sstmac_sumi.hpp>

DIRECT_FN STATIC extern "C" int sstmac_ep_control(fid_t fid, int command, void *arg);
static int sstmac_ep_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
                              void **ops, void *context);

static struct fi_ops sstmac_ep_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_ep_close,
  .bind = sstmac_ep_bind,
  .control = sstmac_ep_control,
  .ops_open = sstmac_ep_ops_open,
};

DIRECT_FN STATIC ssize_t sstmac_ep_rx_size_left(struct fid_ep *ep);
DIRECT_FN STATIC ssize_t sstmac_ep_tx_size_left(struct fid_ep *ep);
static struct fi_ops_ep sstmac_ep_ops = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = sstmac_cancel,
  .getopt = sstmac_getopt,
  .setopt = sstmac_setopt,
  .tx_ctx = fi_no_tx_ctx,
  .rx_ctx = fi_no_rx_ctx,
  .rx_size_left = sstmac_ep_rx_size_left,
  .tx_size_left = sstmac_ep_tx_size_left,
};

DIRECT_FN STATIC ssize_t sstmac_ep_recv(struct fid_ep *ep, void *buf, size_t len,
              void *desc, fi_addr_t src_addr,
              void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_recvv(struct fid_ep *ep,
               const struct iovec *iov,
               void **desc, size_t count,
               fi_addr_t src_addr,
               void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_recvmsg(struct fid_ep *ep,
           const struct fi_msg *msg,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_ep_send(struct fid_ep *ep, const void *buf,
              size_t len, void *desc,
              fi_addr_t dest_addr, void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_sendv(struct fid_ep *ep,
               const struct iovec *iov,
               void **desc, size_t count,
               fi_addr_t dest_addr,
               void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_sendmsg(struct fid_ep *ep,
           const struct fi_msg *msg,
           uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_ep_msg_inject(struct fid_ep *ep, const void *buf,
              size_t len, fi_addr_t dest_addr);
DIRECT_FN STATIC ssize_t sstmac_ep_senddata(struct fid_ep *ep, const void *buf,
            size_t len, void *desc, uint64_t data,
            fi_addr_t dest_addr, void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
           uint64_t data, fi_addr_t dest_addr);
static struct fi_ops_msg sstmac_ep_msg_ops = {
  .size = sizeof(struct fi_ops_msg),
  .recv = sstmac_ep_recv,
  .recvv = sstmac_ep_recvv,
  .recvmsg = sstmac_ep_recvmsg,
  .send = sstmac_ep_send,
  .sendv = sstmac_ep_sendv,
  .sendmsg = sstmac_ep_sendmsg,
  .inject = sstmac_ep_msg_inject,
  .senddata = sstmac_ep_senddata,
  .injectdata = sstmac_ep_msg_injectdata,
};

DIRECT_FN STATIC ssize_t sstmac_ep_read(struct fid_ep *ep, void *buf, size_t len,
              void *desc, fi_addr_t src_addr, uint64_t addr,
              uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
        size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
        void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_ep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
        fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
         size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
         void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
        uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_ep_rma_inject(struct fid_ep *ep, const void *buf,
              size_t len, fi_addr_t dest_addr,
              uint64_t addr, uint64_t key);
DIRECT_FN STATIC ssize_t
sstmac_ep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
      uint64_t data, fi_addr_t dest_addr, uint64_t addr,
      uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
           uint64_t data, fi_addr_t dest_addr, uint64_t addr,
           uint64_t key);
static struct fi_ops_rma sstmac_ep_rma_ops = {
  .size = sizeof(struct fi_ops_rma),
  .read = sstmac_ep_read,
  .readv = sstmac_ep_readv,
  .readmsg = sstmac_ep_readmsg,
  .write = sstmac_ep_write,
  .writev = sstmac_ep_writev,
  .writemsg = sstmac_ep_writemsg,
  .inject = sstmac_ep_rma_inject,
  .writedata = sstmac_ep_writedata,
  .injectdata = sstmac_ep_rma_injectdata,
};

DIRECT_FN STATIC ssize_t sstmac_ep_trecv(struct fid_ep *ep, void *buf, size_t len,
               void *desc, fi_addr_t src_addr,
               uint64_t tag, uint64_t ignore,
               void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_trecvv(struct fid_ep *ep,
          const struct iovec *iov,
          void **desc, size_t count,
          fi_addr_t src_addr,
          uint64_t tag, uint64_t ignore,
          void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_trecvmsg(struct fid_ep *ep,
            const struct fi_msg_tagged *msg,
            uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_ep_tsend(struct fid_ep *ep, const void *buf,
               size_t len, void *desc,
               fi_addr_t dest_addr, uint64_t tag,
               void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_tsendv(struct fid_ep *ep,
          const struct iovec *iov,
          void **desc, size_t count,
          fi_addr_t dest_addr,
          uint64_t tag, void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_tsendmsg(struct fid_ep *ep,
            const struct fi_msg_tagged *msg,
            uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_ep_tinject(struct fid_ep *ep, const void *buf,
           size_t len, fi_addr_t dest_addr,
           uint64_t tag);
DIRECT_FN STATIC ssize_t sstmac_ep_tinjectdata(struct fid_ep *ep, const void *buf,
               size_t len, uint64_t data,
               fi_addr_t dest_addr, uint64_t tag);
DIRECT_FN STATIC ssize_t sstmac_ep_tsenddata(struct fid_ep *ep, const void *buf,
             size_t len, void *desc,
             uint64_t data, fi_addr_t dest_addr,
             uint64_t tag, void *context);
struct fi_ops_tagged sstmac_ep_tagged_ops = {
  .size = sizeof(struct fi_ops_tagged),
  .recv = sstmac_ep_trecv,
  .recvv = sstmac_ep_trecvv,
  .recvmsg = sstmac_ep_trecvmsg,
  .send = sstmac_ep_tsend,
  .sendv = sstmac_ep_tsendv,
  .sendmsg = sstmac_ep_tsendmsg,
  .inject = sstmac_ep_tinject,
  .senddata = sstmac_ep_tsenddata,
  .injectdata = sstmac_ep_tinjectdata,
};

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
         void *desc, fi_addr_t dest_addr, uint64_t addr,
         uint64_t key, enum fi_datatype datatype, enum fi_op op,
         void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
          size_t count, fi_addr_t dest_addr, uint64_t addr,
          uint64_t key, enum fi_datatype datatype, enum fi_op op,
          void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
      uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
          fi_addr_t dest_addr, uint64_t addr, uint64_t key,
          enum fi_datatype datatype, enum fi_op op);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
       void *desc, void *result, void *result_desc,
       fi_addr_t dest_addr, uint64_t addr, uint64_t key,
       enum fi_datatype datatype, enum fi_op op,
       void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
        void **desc, size_t count, struct fi_ioc *resultv,
        void **result_desc, size_t result_count,
        fi_addr_t dest_addr, uint64_t addr, uint64_t key,
        enum fi_datatype datatype, enum fi_op op,
        void *context);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
          struct fi_ioc *resultv, void **result_desc,
          size_t result_count, uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
       void *desc, const void *compare, void *compare_desc,
       void *result, void *result_desc, fi_addr_t dest_addr,
       uint64_t addr, uint64_t key, enum fi_datatype datatype,
       enum fi_op op, void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_atomic_compwritev(struct fid_ep *ep,
               const struct fi_ioc *iov,
               void **desc,
               size_t count,
               const struct fi_ioc *comparev,
               void **compare_desc,
               size_t compare_count,
               struct fi_ioc *resultv,
               void **result_desc,
               size_t result_count,
               fi_addr_t dest_addr,
               uint64_t addr, uint64_t key,
               enum fi_datatype datatype,
               enum fi_op op,
               void *context);
DIRECT_FN STATIC ssize_t sstmac_ep_atomic_compwritemsg(struct fid_ep *ep,
                 const struct fi_msg_atomic *msg,
                 const struct fi_ioc *comparev,
                 void **compare_desc,
                 size_t compare_count,
                 struct fi_ioc *resultv,
                 void **result_desc,
                 size_t result_count,
                 uint64_t flags);

struct fi_ops_atomic sstmac_ep_atomic_ops = {
  .size = sizeof(struct fi_ops_atomic),
  .write = sstmac_ep_atomic_write,
  .writev = sstmac_ep_atomic_writev,
  .writemsg = sstmac_ep_atomic_writemsg,
  .inject = sstmac_ep_atomic_inject,
  .readwrite = sstmac_ep_atomic_readwrite,
  .readwritev = sstmac_ep_atomic_readwritev,
  .readwritemsg = sstmac_ep_atomic_readwritemsg,
  .compwrite = sstmac_ep_atomic_compwrite,
  .compwritev = sstmac_ep_atomic_compwritev,
  .compwritemsg = sstmac_ep_atomic_compwritemsg,
  .writevalid = sstmac_ep_atomic_valid,
  .readwritevalid = sstmac_ep_fetch_atomic_valid,
  .compwritevalid = sstmac_ep_cmp_atomic_valid,
};



ssize_t _ep_recv(struct fid_ep *ep, void *buf, size_t len,
		 void *desc, fi_addr_t src_addr, void *context,
		 uint64_t flags, uint64_t tag, uint64_t ignore)
{
#if 0
	struct sstmac_fid_ep *ep_priv;

	if (!ep) {
		return -FI_EINVAL;
	}

	ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid);

	if (!(ep_priv->op_flags & FI_MULTI_RECV)) {
		return _sstmac_recv(ep_priv,
				  (uint64_t)buf,
				  len, desc,
				  src_addr,
				  context,
				  ep_priv->op_flags | flags,
				  tag,
				  ignore,
				  NULL);
	 } else {
		return _sstmac_recv_mr(ep_priv,
				     (uint64_t)buf,
				     len,
				     desc,
				     src_addr,
				     context,
				     ep_priv->op_flags | flags,
				     tag,
				     ignore);
	}
#endif
  return 0;
}

ssize_t _ep_recvv(struct fid_ep *ep, const struct iovec *iov,
		  void **desc, size_t count, fi_addr_t src_addr,
		  void *context, uint64_t flags, uint64_t tag,
		  uint64_t ignore)
{
#if 0
	struct sstmac_fid_ep *ep_priv;

  if (!ep || !iov || count > SSTMAC_MAX_MSG_IOV_LIMIT) {
		return -FI_EINVAL;
	}

	ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(ep_priv->type));

	if (count <= 1) {
		if (!(ep_priv->op_flags & FI_MULTI_RECV)) {
			return _sstmac_recv(ep_priv,
					  (uint64_t)iov[0].iov_base,
					  iov[0].iov_len,
					  desc ? desc[0] : NULL,
					  src_addr,
					  context,
					  ep_priv->op_flags | flags,
					  tag,
					  ignore,
					  NULL);
		} else {
			return _sstmac_recv_mr(ep_priv,
					  (uint64_t)iov[0].iov_base,
					  iov[0].iov_len,
					  desc ? desc[0] : NULL,
					  src_addr,
					  context,
					  ep_priv->op_flags | flags,
					  tag,
					  ignore);
		}
	}

	return _sstmac_recvv(ep_priv, iov, desc, count, src_addr,
			   context, ep_priv->op_flags | flags, ignore, tag);
#endif
  return 0;
}

ssize_t _ep_recvmsg(struct fid_ep *ep, const struct fi_msg *msg,
		    uint64_t flags, uint64_t tag,
		    uint64_t ignore)
{
#if 0
	struct iovec iov;
	struct sstmac_fid_ep *ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid);

	ep_priv = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(ep_priv->type));

	iov.iov_base = NULL;
	iov.iov_len = 0;

	if (!msg) {
		return -FI_EINVAL;
	}

	if (flags & FI_MULTI_RECV) {
		return _sstmac_recv_mr(ep_priv,
				     (uint64_t)msg->msg_iov[0].iov_base,
				     msg->msg_iov[0].iov_len,
				     msg->desc ? msg->desc[0] : NULL,
				     msg->addr,
				     msg->context,
				     ep_priv->op_flags | flags,
				     tag,
				     ignore);
	}

	/* msg_iov can be undefined when using FI_PEEK, etc. */
	return _ep_recvv(ep, msg->msg_iov ? msg->msg_iov : &iov, msg->desc,
			 msg->iov_count, msg->addr, msg->context, flags, tag,
			 ignore);
#endif
  return 0;
}

ssize_t _ep_send(struct fid_ep *ep, const void *buf, size_t len,
		 void *desc, fi_addr_t dest_addr, void *context,
		 uint64_t flags, uint64_t tag)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	return _sstmac_send(sstmac_ep, (uint64_t)buf, len, desc, dest_addr, context,
			  sstmac_ep->op_flags | flags, 0, tag);
#endif
  return 0;
}

ssize_t _ep_sendv(struct fid_ep *ep, const struct iovec *iov,
		  void **desc, size_t count, fi_addr_t dest_addr,
		  void *context, uint64_t flags, uint64_t tag)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

  if (!ep || !iov || !count || count > SSTMAC_MAX_MSG_IOV_LIMIT) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	if (count == 1) {
		return _sstmac_send(sstmac_ep, (uint64_t)iov[0].iov_base,
				  iov[0].iov_len, desc ? desc[0] : NULL,
				  dest_addr, context, sstmac_ep->op_flags | flags,
				  0, tag);
	}

	return _sstmac_sendv(sstmac_ep, iov, desc, count, dest_addr, context,
			   sstmac_ep->op_flags | flags, tag);
#endif
  return 0;
}

ssize_t _ep_sendmsg(struct fid_ep *ep, const struct fi_msg *msg,
		     uint64_t flags, uint64_t tag)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (!ep || !msg || !msg->msg_iov || !msg->iov_count) {
		return -FI_EINVAL;
	}

	/* Must check the iov count here, can't send msg->data to sendv */
	if (msg->iov_count > 1) {
		return _ep_sendv(ep, msg->msg_iov, msg->desc, msg->iov_count,
				 msg->addr, msg->context, flags, tag);
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	return _sstmac_send(sstmac_ep, (uint64_t)msg->msg_iov[0].iov_base,
			  msg->msg_iov[0].iov_len,
			  msg->desc ? msg->desc[0] : NULL, msg->addr,
			  msg->context, flags, msg->data, tag);
#endif
  return 0;
}

ssize_t _ep_inject(struct fid_ep *ep, const void *buf,
		   size_t len, uint64_t data, fi_addr_t dest_addr,
		   uint64_t flags, uint64_t tag)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t inject_flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	inject_flags = (sstmac_ep->op_flags | FI_INJECT |
      SSTMAC_SUPPRESS_COMPLETION | flags);

	return _sstmac_send(sstmac_ep, (uint64_t)buf, len, NULL, dest_addr,
			  NULL, inject_flags, data, tag);
#endif
  return 0;
}

ssize_t _ep_senddata(struct fid_ep *ep, const void *buf,
		     size_t len, void *desc, uint64_t data,
		     fi_addr_t dest_addr, void *context,
		     uint64_t flags, uint64_t tag)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t sd_flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	sd_flags = sstmac_ep->op_flags | FI_REMOTE_CQ_DATA | flags;

	return _sstmac_send(sstmac_ep, (uint64_t)buf, len, desc, dest_addr,
			  context, sd_flags, data, tag);
#endif
  return 0;
}


DIRECT_FN STATIC ssize_t sstmac_ep_recv(struct fid_ep *ep, void *buf, size_t len,
				      void *desc, fi_addr_t src_addr,
				      void *context)
{
	return _ep_recv(ep, buf, len, desc, src_addr, context, 0, 0, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_recvv(struct fid_ep *ep,
				       const struct iovec *iov,
				       void **desc, size_t count,
				       fi_addr_t src_addr,
				       void *context)
{
	return _ep_recvv(ep, iov, desc, count, src_addr, context, 0, 0, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_recvmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags)
{
  return _ep_recvmsg(ep, msg, flags & SSTMAC_RECVMSG_FLAGS, 0, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_send(struct fid_ep *ep, const void *buf,
				      size_t len, void *desc,
				      fi_addr_t dest_addr, void *context)
{
	return _ep_send(ep, buf, len, desc, dest_addr, context, 0, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_sendv(struct fid_ep *ep,
				       const struct iovec *iov,
				       void **desc, size_t count,
				       fi_addr_t dest_addr,
				       void *context)
{
	return _ep_sendv(ep, iov, desc, count, dest_addr, context, 0, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_sendmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags)
{
  return _ep_sendmsg(ep, msg, flags & SSTMAC_SENDMSG_FLAGS, 0);
}

DIRECT_FN STATIC ssize_t sstmac_ep_msg_inject(struct fid_ep *ep, const void *buf,
					    size_t len, fi_addr_t dest_addr)
{
	return _ep_inject(ep, buf, len, 0, dest_addr, 0, 0);
}

static ssize_t sstmaci_ep_send(struct fid_ep* ep, const void* buf, size_t len,
                               fi_addr_t dest_addr, void* context,
                               uint64_t data, uint64_t flags)
{
  sstmac_fid_ep* ep_impl = (sstmac_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  uint32_t dest_rank = ADDR_RANK(dest_addr);
  uint16_t remote_cq = ADDR_CQ(dest_addr);
  uint16_t recv_queue = ADDR_QUEUE(dest_addr);

  flags |= FI_SEND;

  tport->postSend<FabricMessage>(dest_rank, len, const_cast<void*>(buf),
                                 ep_impl->send_cq->id, // rma operations go to the tx
                                 remote_cq, recv_queue,
                                 sumi::Message::pt2pt, ep_impl->qos,
                                 FabricMessage::no_tag, FabricMessage::no_imm_data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_senddata(struct fid_ep *ep, const void *buf,
					  size_t len, void *desc, uint64_t data,
					  fi_addr_t dest_addr, void *context)
{
	return _ep_senddata(ep, buf, len, desc, data, dest_addr, context, 0, 0);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		       uint64_t data, fi_addr_t dest_addr)
{
  return -FI_ENOSYS;
}

static ssize_t sstmaci_ep_read(struct fid_ep *ep, void *buf, size_t len,
                               fi_addr_t src_addr, uint64_t addr,
                               void *context)
{
  sstmac_fid_ep* ep_impl = (sstmac_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  uint32_t src_rank = ADDR_RANK(src_addr);

  uint64_t flags = 0;
  int remote_cq = sumi::Message::no_ack;
  if (ep_impl->op_flags & FI_REMOTE_READ){
    remote_cq = ADDR_CQ(src_addr);
    flags |= FI_REMOTE_READ;
  }
  flags |= FI_READ;

  tport->rdmaGet<FabricMessage>(src_rank, len, buf, (void*) addr,
                                ep_impl->send_cq->id, // rma operations go to the tx
                                remote_cq,
                                sumi::Message::pt2pt, ep_impl->qos,
                                FabricMessage::no_tag, FabricMessage::no_imm_data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_read(struct fid_ep *ep, void *buf, size_t len,
				      void *desc, fi_addr_t src_addr, uint64_t addr,
				      uint64_t key, void *context)
{
  return sstmaci_ep_read(ep, buf, len, src_addr, addr, context);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	      size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
	      void *context)
{
  if (count == 1){
    return sstmaci_ep_read(ep, iov[0].iov_base, iov[0].iov_len, src_addr, addr, context);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t
sstmac_ep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags)
{
  return -FI_ENOSYS;
}

static ssize_t sstmaci_ep_write(struct fid_ep *ep, const void *buf, size_t len,
                                fi_addr_t dest_addr, uint64_t addr, void *context,
                                uint64_t data, uint64_t flags)
{
  sstmac_fid_ep* ep_impl = (sstmac_fid_ep*) ep;
  FabricTransport* tport = (FabricTransport*) ep_impl->domain->fabric->tport;

  int remote_cq = sumi::Message::no_ack;
  if (ep_impl->op_flags & FI_REMOTE_WRITE){
    remote_cq = ADDR_CQ(dest_addr);
    flags |= FI_REMOTE_WRITE;
  }
  flags |= FI_WRITE;

  uint32_t src_rank = ADDR_RANK(dest_addr);
  tport->rdmaPut<FabricMessage>(src_rank, len, const_cast<void*>(buf), (void*) addr,
                                ep_impl->send_cq->id, // rma operations go to the tx
                                remote_cq,
                                sumi::Message::pt2pt, ep_impl->qos,
                                FabricMessage::no_tag, data, flags, context);
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	      fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context)
{
  return sstmaci_ep_write(ep, buf, len, dest_addr, addr, context,
                          FabricMessage::no_imm_data, FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
	       size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
	       void *context)
{
  if (count == 1){
    return sstmaci_ep_write(ep, iov[0].iov_base, iov[0].iov_len, dest_addr, addr, context,
                            FabricMessage::no_imm_data, 0);
  } else {
    return -FI_ENOSYS;
  }
}

DIRECT_FN STATIC ssize_t sstmac_ep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
				uint64_t flags)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sstmac_ep_rma_inject(struct fid_ep *ep, const void *buf,
					    size_t len, fi_addr_t dest_addr,
					    uint64_t addr, uint64_t key)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		  uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		  uint64_t key, void *context)
{
  return sstmaci_ep_write(ep, buf, len, dest_addr, addr, context, data, FI_REMOTE_CQ_DATA);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		       uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		       uint64_t key)
{
  return -FI_ENOSYS;
}

DIRECT_FN STATIC ssize_t sstmac_ep_trecv(struct fid_ep *ep, void *buf, size_t len,
				       void *desc, fi_addr_t src_addr,
				       uint64_t tag, uint64_t ignore,
				       void *context)
{
	return _ep_recv(ep, buf, len, desc, src_addr, context,
			FI_TAGGED, tag, ignore);
}

DIRECT_FN STATIC ssize_t sstmac_ep_trecvv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t src_addr,
					uint64_t tag, uint64_t ignore,
					void *context)
{
	return _ep_recvv(ep, iov, desc, count, src_addr, context,
			  FI_TAGGED, tag, ignore);
}

DIRECT_FN STATIC ssize_t sstmac_ep_trecvmsg(struct fid_ep *ep,
					  const struct fi_msg_tagged *msg,
					  uint64_t flags)
{
	const struct fi_msg _msg = {
			.msg_iov = msg->msg_iov,
			.desc = msg->desc,
			.iov_count = msg->iov_count,
			.addr = msg->addr,
			.context = msg->context,
			.data = msg->data
	};

  if (flags & ~SSTMAC_TRECVMSG_FLAGS)
		return -FI_EINVAL;

	/* From the fi_tagged man page regarding the use of FI_CLAIM:
	 *
	 * In order to use the FI_CLAIM flag, an  application  must  supply  a
	 * struct  fi_context  structure as the context for the receive opera-
	 * tion.  The same fi_context structure used for an FI_PEEK + FI_CLAIM
	 * operation must be used by the paired FI_CLAIM requests
	 */
	if ((flags & FI_CLAIM) && _msg.context == NULL)
		return -FI_EINVAL;

	/* From the fi_tagged man page regarding the use of FI_DISCARD:
	 *
	 * This  flag  must  be used in conjunction with either
	 * FI_PEEK or FI_CLAIM.
	 *
	 * Note: I suspect the use of all three flags at the same time is invalid,
	 * but the man page does not say that it is.
	 */
	if ((flags & FI_DISCARD) && !(flags & (FI_PEEK | FI_CLAIM)))
		return -FI_EINVAL;

	return _ep_recvmsg(ep, &_msg, flags | FI_TAGGED, msg->tag,
			msg->ignore);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tsend(struct fid_ep *ep, const void *buf,
				       size_t len, void *desc,
				       fi_addr_t dest_addr, uint64_t tag,
				       void *context)
{
	return _ep_send(ep, buf, len, desc, dest_addr, context,
			FI_TAGGED, tag);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tsendv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t dest_addr,
					uint64_t tag, void *context)
{
	return _ep_sendv(ep, iov, desc, count, dest_addr, context,
			 FI_TAGGED, tag);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tsendmsg(struct fid_ep *ep,
					  const struct fi_msg_tagged *msg,
					  uint64_t flags)
{
	const struct fi_msg _msg = {
			.msg_iov = msg->msg_iov,
			.desc = msg->desc,
			.iov_count = msg->iov_count,
			.addr = msg->addr,
			.context = msg->context,
			.data = msg->data
	};

  if (flags & ~(SSTMAC_SENDMSG_FLAGS))
		return -FI_EINVAL;

	return _ep_sendmsg(ep, &_msg, flags | FI_TAGGED, msg->tag);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tinject(struct fid_ep *ep, const void *buf,
					 size_t len, fi_addr_t dest_addr,
					 uint64_t tag)
{
	return _ep_inject(ep, buf, len, 0, dest_addr, FI_TAGGED, tag);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tsenddata(struct fid_ep *ep, const void *buf,
					   size_t len, void *desc,
					   uint64_t data, fi_addr_t dest_addr,
					   uint64_t tag, void *context)
{
	return _ep_senddata(ep, buf, len, desc, data, dest_addr, context,
			FI_TAGGED, tag);
}

DIRECT_FN STATIC ssize_t sstmac_ep_tinjectdata(struct fid_ep *ep, const void *buf,
					     size_t len, uint64_t data,
					     fi_addr_t dest_addr, uint64_t tag)
{
	return _ep_inject(ep, buf, len, data, dest_addr,
			  FI_TAGGED | FI_REMOTE_CQ_DATA, tag);
}

DIRECT_FN extern "C" int sstmac_ep_atomic_valid(struct fid_ep *ep,
					  enum fi_datatype datatype,
					  enum fi_op op, size_t *count)
{
  return 0;
}

DIRECT_FN extern "C" int sstmac_ep_fetch_atomic_valid(struct fid_ep *ep,
						enum fi_datatype datatype,
						enum fi_op op, size_t *count)
{
  return 0;
}

DIRECT_FN extern "C" int sstmac_ep_cmp_atomic_valid(struct fid_ep *ep,
					      enum fi_datatype datatype,
					      enum fi_op op, size_t *count)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
		     void *desc, fi_addr_t dest_addr, uint64_t addr,
		     uint64_t key, enum fi_datatype datatype, enum fi_op op,
		     void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	struct fi_msg_atomic msg;
	struct fi_ioc msg_iov;
	struct fi_rma_ioc rma_iov;
	uint64_t flags;

	if (sstmac_ep_atomic_valid(ep, datatype, op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	msg_iov.addr = (void *)buf;
	msg_iov.count = count;
	msg.msg_iov = &msg_iov;
	msg.desc = &desc;
	msg.iov_count = 1;
	msg.addr = dest_addr;
	rma_iov.addr = addr;
	rma_iov.count = 1;
	rma_iov.key = key;
	msg.rma_iov = &rma_iov;
	msg.datatype = datatype;
	msg.op = op;
	msg.context = context;

  flags = sstmac_ep->op_flags | SSTMAC_ATOMIC_WRITE_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_AMO, &msg,
			    NULL, NULL, 0, NULL, NULL, 0, flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
		      size_t count, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context)
{
#if 0
	if (!iov || count > 1) {
		return -FI_EINVAL;
	}
#endif
	return sstmac_ep_atomic_write(ep, iov[0].addr, iov[0].count,
				    desc ? desc[0] : NULL,
				    dest_addr, addr, key, datatype, op,
				    context);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			uint64_t flags)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (sstmac_ep_atomic_valid(ep, msg->datatype, msg->op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = (flags & SSTMAC_ATOMICMSG_FLAGS) | SSTMAC_ATOMIC_WRITE_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_AMO, msg,
			    NULL, NULL, 0, NULL, NULL, 0, flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
		      fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		      enum fi_datatype datatype, enum fi_op op)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	struct fi_msg_atomic msg;
	struct fi_ioc msg_iov;
	struct fi_rma_ioc rma_iov;
	uint64_t flags;

	if (sstmac_ep_atomic_valid(ep, datatype, op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	msg_iov.addr = (void *)buf;
	msg_iov.count = count;
	msg.msg_iov = &msg_iov;
	msg.desc = NULL;
	msg.iov_count = 1;
	msg.addr = dest_addr;
	rma_iov.addr = addr;
	rma_iov.count = 1;
	rma_iov.key = key;
	msg.rma_iov = &rma_iov;
	msg.datatype = datatype;
	msg.op = op;

  flags = sstmac_ep->op_flags | FI_INJECT | SSTMAC_SUPPRESS_COMPLETION |
      SSTMAC_ATOMIC_WRITE_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_AMO, &msg,
			    NULL, NULL, 0, NULL, NULL, 0, flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, void *result, void *result_desc,
			 fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			 enum fi_datatype datatype, enum fi_op op,
			 void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	struct fi_msg_atomic msg;
	struct fi_ioc msg_iov;
	struct fi_rma_ioc rma_iov;
	struct fi_ioc result_iov;
	uint64_t flags;

	if (sstmac_ep_fetch_atomic_valid(ep, datatype, op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	msg_iov.addr = (void *)buf;
	msg_iov.count = count;
	msg.msg_iov = &msg_iov;
	msg.desc = &desc;
	msg.iov_count = 1;
	msg.addr = dest_addr;
	rma_iov.addr = addr;
	rma_iov.count = 1;
	rma_iov.key = key;
	msg.rma_iov = &rma_iov;
	msg.datatype = datatype;
	msg.op = op;
	msg.context = context;
	result_iov.addr = result;
	result_iov.count = 1;

  flags = sstmac_ep->op_flags | SSTMAC_ATOMIC_READ_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_FAMO, &msg,
			    NULL, NULL, 0,
			    &result_iov, &result_desc, 1,
			    flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			  void **desc, size_t count, struct fi_ioc *resultv,
			  void **result_desc, size_t result_count,
			  fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context)
{
	if (!iov || count > 1 || !resultv)
		return -FI_EINVAL;

	return sstmac_ep_atomic_readwrite(ep, iov[0].addr, iov[0].count,
					desc ? desc[0] : NULL,
					resultv[0].addr,
					result_desc ? result_desc[0] : NULL,
					dest_addr, addr, key, datatype, op,
					context);
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			    struct fi_ioc *resultv, void **result_desc,
			    size_t result_count, uint64_t flags)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (sstmac_ep_fetch_atomic_valid(ep, msg->datatype, msg->op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = (flags & SSTMAC_FATOMICMSG_FLAGS) | SSTMAC_ATOMIC_READ_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_FAMO, msg,
			    NULL, NULL, 0,
			    resultv, result_desc, result_count,
			    flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, const void *compare, void *compare_desc,
			 void *result, void *result_desc, fi_addr_t dest_addr,
			 uint64_t addr, uint64_t key, enum fi_datatype datatype,
			 enum fi_op op, void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	struct fi_msg_atomic msg;
	struct fi_ioc msg_iov;
	struct fi_rma_ioc rma_iov;
	struct fi_ioc result_iov;
	struct fi_ioc compare_iov;
	uint64_t flags;

	if (sstmac_ep_cmp_atomic_valid(ep, datatype, op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	msg_iov.addr = (void *)buf;
	msg_iov.count = count;
	msg.msg_iov = &msg_iov;
	msg.desc = &desc;
	msg.iov_count = 1;
	msg.addr = dest_addr;
	rma_iov.addr = addr;
	rma_iov.count = 1;
	rma_iov.key = key;
	msg.rma_iov = &rma_iov;
	msg.datatype = datatype;
	msg.op = op;
	msg.context = context;
	result_iov.addr = result;
	result_iov.count = 1;
	compare_iov.addr = (void *)compare;
	compare_iov.count = 1;

  flags = sstmac_ep->op_flags | SSTMAC_ATOMIC_READ_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_CAMO, &msg,
			    &compare_iov, &compare_desc, 1,
			    &result_iov, &result_desc, 1,
			    flags);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_atomic_compwritev(struct fid_ep *ep,
						   const struct fi_ioc *iov,
						   void **desc,
						   size_t count,
						   const struct fi_ioc *comparev,
						   void **compare_desc,
						   size_t compare_count,
						   struct fi_ioc *resultv,
						   void **result_desc,
						   size_t result_count,
						   fi_addr_t dest_addr,
						   uint64_t addr, uint64_t key,
						   enum fi_datatype datatype,
						   enum fi_op op,
						   void *context)
{
#if 0
	if (!iov || count > 1 || !resultv || !comparev)
		return -FI_EINVAL;

	return sstmac_ep_atomic_compwrite(ep, iov[0].addr, iov[0].count,
					desc ? desc[0] : NULL,
					comparev[0].addr,
					compare_desc ? compare_desc[0] : NULL,
					resultv[0].addr,
					result_desc ? result_desc[0] : NULL,
					dest_addr, addr, key, datatype, op,
					context);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_atomic_compwritemsg(struct fid_ep *ep,
						     const struct fi_msg_atomic *msg,
						     const struct fi_ioc *comparev,
						     void **compare_desc,
						     size_t compare_count,
						     struct fi_ioc *resultv,
						     void **result_desc,
						     size_t result_count,
						     uint64_t flags)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (sstmac_ep_cmp_atomic_valid(ep, msg->datatype, msg->op, NULL))
		return -FI_EOPNOTSUPP;

	if (!ep)
		return -FI_EINVAL;

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = (flags & SSTMAC_CATOMICMSG_FLAGS) | SSTMAC_ATOMIC_READ_FLAGS_DEF;

  return _sstmac_atomic(sstmac_ep, SSTMAC_FAB_RQ_CAMO, msg,
			    comparev, compare_desc, compare_count,
			    resultv, result_desc, result_count,
			    flags);
#endif
  return 0;
}

DIRECT_FN STATIC extern "C" int sstmac_ep_control(fid_t fid, int command, void *arg)
{
#if 0
	int ret = FI_SUCCESS;
	struct sstmac_fid_ep *ep;

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	ep = container_of(fid, struct sstmac_fid_ep, ep_fid);

	switch (command) {
	/*
	 * for FI_EP_RDM/DGRAM, enable the cm_nic associated
	 * with this ep.
	 */
	case FI_ENABLE:

    if (SSTMAC_EP_RDM_DGM(ep->type)) {
			if ((ep->send_cq && ep->tx_enabled)) {
				ret = -FI_EOPBADSTATE;
				goto err;
			}
			if ((ep->recv_cq && ep->rx_enabled)) {
				ret = -FI_EOPBADSTATE;
				goto err;
			}
			ret = _sstmac_vc_cm_init(ep->cm_nic);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
				     "_sstmac_vc_cm_nic_init call returned %d\n",
					ret);
				goto err;
			}
			ret = _sstmac_cm_nic_enable(ep->cm_nic);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
				     "_sstmac_cm_nic_enable call returned %d\n",
					ret);
				goto err;
			}
		}

		ret = _sstmac_ep_tx_enable(ep);
		if (ret != FI_SUCCESS) {
      SSTMAC_WARN(FI_LOG_EP_CTRL,
			     "_sstmac_ep_tx_enable call returned %d\n",
				ret);
			goto err;
		}

		ret = _sstmac_ep_rx_enable(ep);
		if (ret != FI_SUCCESS) {
      SSTMAC_WARN(FI_LOG_EP_CTRL,
			     "_sstmac_ep_rx_enable call returned %d\n",
				ret);
			goto err;
		}

		ret = _sstmac_ep_int_tx_pool_init(ep);
		break;

	case FI_GETOPSFLAG:
	case FI_SETOPSFLAG:
	case FI_ALIAS:
	default:
		return -FI_ENOSYS;
	}

err:
	return ret;
#endif
  return 0;
}

extern "C" int sstmac_ep_close(fid_t fid)
{
  sstmac_fid_ep* ep = (sstmac_fid_ep*) fid;
  free(ep);
  return FI_SUCCESS;
}

DIRECT_FN extern "C" int sstmac_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
  //this can always be cast to an endpiont regardless of whether
  //it is a simple endpoint or tx/rx context
  sstmac_fid_ep* ep = (sstmac_fid_ep*) fid;
  FabricTransport* tport = (FabricTransport*) ep->domain->fabric->tport;
  switch(bfid->fclass)
  {
    case FI_CLASS_EQ: {
      sstmac_fid_eq* eq = (sstmac_fid_eq*) bfid;
      if (ep->domain->fabric != eq->fabric) {
        return -FI_EINVAL;
      }
      ep->eq = eq;
      break;
    }
    case FI_CLASS_CQ: {
      sstmac_fid_cq* cq = (sstmac_fid_cq*) bfid;

      if (ep->domain != cq->domain) {
        return -FI_EINVAL;
      }

      if (flags & FI_TRANSMIT) {
        if (ep->send_cq) {
          return -FI_EINVAL; //can't rebind send CQ
        }
        ep->send_cq = cq;
        if (flags & FI_SELECTIVE_COMPLETION) {
          //for now, don't allow selective completion
          //all messages will generate completion entries
          return -FI_EINVAL;
        }
      }

      if (flags & FI_RECV) {
        if (ep->recv_cq) {
          return -FI_EINVAL;
        }
        //TODO ep->rx_id = tport->allocateRecvQueue(ep->recv_cq->id);
        ep->recv_cq = cq;
        if (flags & FI_SELECTIVE_COMPLETION) {
          //for now, don't allow selective completion
          //all messages will generate completion entries
          return -FI_EINVAL;
        }
      }
      break;
    }
    case FI_CLASS_AV: {
      sstmac_fid_av* av = (sstmac_fid_av*) bfid;
      if (ep->domain != av->domain) {
        return -FI_EINVAL;
      }
      break;
    }
    case FI_CLASS_CNTR: //TODO
      return -FI_EINVAL;
    case FI_CLASS_MR: //TODO
      return -FI_EINVAL;
    case FI_CLASS_SRX_CTX:

    case FI_CLASS_STX_CTX:
      //we really don't need to do anything here
      //in some sense, all sstmacro endpoints are shared transmit contexts
      break;
    default:
      return -FI_ENOSYS;
      break;

  }
  return FI_SUCCESS;
}


DIRECT_FN extern "C" int sstmac_ep_open(struct fid_domain *domain, struct fi_info *info,
			   struct fid_ep **ep, void *context)
{
  sstmac_fid_ep* ep_impl = (sstmac_fid_ep*) calloc(1, sizeof(sstmac_fid_ep));
  ep_impl->ep_fid.fid.fclass = FI_CLASS_EP;
  ep_impl->ep_fid.fid.context = context;
  ep_impl->ep_fid.fid.ops = &sstmac_ep_fi_ops;
  ep_impl->ep_fid.ops = &sstmac_ep_ops;
  ep_impl->ep_fid.msg = &sstmac_ep_msg_ops;
  ep_impl->ep_fid.rma = &sstmac_ep_rma_ops;
  ep_impl->ep_fid.tagged = &sstmac_ep_tagged_ops;
  ep_impl->ep_fid.atomic = &sstmac_ep_atomic_ops;
  ep_impl->domain = (sstmac_fid_domain*) domain;
  ep_impl->caps = info->caps;
  if (info->tx_attr){
    ep_impl->op_flags = info->tx_attr->op_flags;
  }
  if (info->rx_attr){
    ep_impl->op_flags = info->rx_attr->op_flags;
  }

  ep_impl->type = info->ep_attr->type;

  switch(ep_impl->type){
  case FI_EP_DGRAM:
  case FI_EP_RDM:
  case FI_EP_MSG:
    break;
  default:
    return -FI_EINVAL;
  }

  *ep = (fid_ep*) ep_impl;
  return FI_SUCCESS;
}

DIRECT_FN STATIC ssize_t sstmac_ep_cancel(fid_t fid, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_ep *ep;
	struct sstmac_fab_req *req;
	struct sstmac_fid_cq *err_cq = NULL;
	struct sstmac_fid_cntr *err_cntr = NULL;
	void *addr;
	uint64_t tag, flags;
	size_t len;
	int is_send = 0;

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);

	if (!ep->domain)
		return -FI_EDOMAIN;

	/* without context, we will have to find a request that matches
	 * a recv or send request. Try the send requests first.
	 */
  SSTMAC_INFO(FI_LOG_EP_CTRL, "looking for event to cancel\n");

	req = __find_tx_req(ep, context);
	if (!req) {
		req = __find_rx_req(ep, context);
		if (req) {
			err_cq = ep->recv_cq;
			err_cntr = ep->recv_cntr;
		}
	} else {
		is_send = 1;
		err_cq = ep->send_cq;
		err_cntr = ep->send_cntr;
	}
  SSTMAC_INFO(FI_LOG_EP_CTRL, "finished searching\n");

	if (!req)
		return -FI_ENOENT;

	if (err_cq) {
		/* add canceled event */
    if (!(req->type == SSTMAC_FAB_RQ_RDMA_READ ||
          req->type == SSTMAC_FAB_RQ_RDMA_WRITE)) {
			if (!is_send) {
				addr = (void *) req->msg.recv_info[0].recv_addr;
				len = req->msg.cum_recv_len;
			} else {
				addr = (void *) req->msg.send_info[0].send_addr;
				len = req->msg.cum_send_len;
			}
			tag = req->msg.tag;
		} else {
			/* rma information */
			addr = (void *) req->rma.loc_addr;
			len = req->rma.len;
			tag = 0;
		}
		flags = req->flags;

		_sstmac_cq_add_error(err_cq, context, flags, len, addr, 0 /* data */,
				tag, len, FI_ECANCELED, FI_ECANCELED, 0, 0);

	}

	if (err_cntr) {
		/* signal increase in cntr errs */
		_sstmac_cntr_inc_err(err_cntr);
	}

	if (req->flags & FI_LOCAL_MR) {
		fi_close(&req->amo.loc_md->mr_fid.fid);
		req->flags &= ~FI_LOCAL_MR;
	}

	_sstmac_fr_free(ep, req);
#endif
	return ret;
}

ssize_t sstmac_cancel(fid_t fid, void *context)
{
	ssize_t ret;
#if 0
	struct sstmac_fid_ep *ep;
	struct sstmac_fid_trx *trx_ep;

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	switch (fid->fclass) {
	case FI_CLASS_EP:
		ret = sstmac_ep_cancel(fid, context);
		break;

	case FI_CLASS_RX_CTX:
	case FI_CLASS_TX_CTX:
		trx_ep = container_of(fid, struct sstmac_fid_trx, ep_fid);
		ep = trx_ep->ep;
		ret = sstmac_ep_cancel(&ep->ep_fid.fid, context);
		break;
	/* not supported yet */
	case FI_CLASS_SRX_CTX:
	case FI_CLASS_STX_CTX:
		return -FI_ENOENT;

	default:
    SSTMAC_WARN(FI_LOG_EP_CTRL, "Invalid fid type\n");
		return -FI_EINVAL;
	}
#endif
	return ret;
}

static int
sstmac_ep_ops_open(struct fid *fid, const char *ops_name, uint64_t flags,
		     void **ops, void *context)
{
  return -FI_EINVAL;
}

DIRECT_FN STATIC extern "C" int sstmac_ep_getopt(fid_t fid, int level, int optname,
				    void *optval, size_t *optlen)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	sstmac_ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);

	switch (optname) {
	case FI_OPT_MIN_MULTI_RECV:
		*(size_t *)optval = sstmac_ep->min_multi_recv;
		*optlen = sizeof(size_t);
		break;
	case FI_OPT_CM_DATA_SIZE:
    *(size_t *)optval = SSTMAC_CM_DATA_MAX_SIZE;
		*optlen = sizeof(size_t);
		break;
	default:
		return -FI_ENOPROTOOPT;
	}
#endif
	return 0;
}

extern "C" int sstmac_getopt(fid_t fid, int level, int optname,
				    void *optval, size_t *optlen)
{
        ssize_t ret;
#if 0
        struct sstmac_fid_ep *ep;
        struct sstmac_fid_trx *trx_ep;

        SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	if (!fid || !optval || !optlen)
		return -FI_EINVAL;
	else if (level != FI_OPT_ENDPOINT)
		return -FI_ENOPROTOOPT;

        switch (fid->fclass) {
        case FI_CLASS_EP:
                ret = sstmac_ep_getopt(fid, level, optname, optval, optlen);
                break;

        case FI_CLASS_RX_CTX:
        case FI_CLASS_TX_CTX:
                trx_ep = container_of(fid, struct sstmac_fid_trx, ep_fid);
                ep = trx_ep->ep;
                ret = sstmac_ep_getopt(&ep->ep_fid.fid, level, optname, optval,
                        optlen);
                break;
        /* not supported yet */
        case FI_CLASS_SRX_CTX:
        case FI_CLASS_STX_CTX:
                return -FI_ENOENT;

        default:
                SSTMAC_WARN(FI_LOG_EP_CTRL, "Invalid fid type\n");
                return -FI_EINVAL;
        }
#endif
        return ret;
}

DIRECT_FN STATIC extern "C" int sstmac_ep_setopt(fid_t fid, int level, int optname,
				    const void *optval, size_t optlen)
{
  return -FI_ENOPROTOOPT;
}

extern "C" int sstmac_setopt(fid_t fid, int level, int optname,
				    const void *optval, size_t optlen)
{
  return -FI_EINVAL;
}

DIRECT_FN STATIC ssize_t sstmac_ep_rx_size_left(struct fid_ep *ep)
{
#if 0
	if (!ep) {
		return -FI_EINVAL;
	}

	struct sstmac_fid_ep *ep_priv = container_of(ep,
						   struct sstmac_fid_ep,
						   ep_fid);

	/* A little arbitrary... */
	if (ep_priv->int_tx_pool.enabled == false) {
		return -FI_EOPBADSTATE;
	}

	switch (ep->fid.fclass) {
	case FI_CLASS_EP:
		break;
	case FI_CLASS_RX_CTX:
	case FI_CLASS_SRX_CTX:
		break;
	default:
    SSTMAC_INFO(FI_LOG_EP_CTRL, "Invalid EP type\n");
		return -FI_EINVAL;
	}

	/* We can queue RXs indefinitely, so just return the default size. */
#endif
  return SSTMAC_RX_SIZE_DEFAULT;
}

DIRECT_FN STATIC ssize_t sstmac_ep_tx_size_left(struct fid_ep *ep)
{
#if 0
	if (!ep) {
		return -FI_EINVAL;
	}

	struct sstmac_fid_ep *ep_priv = container_of(ep,
						   struct sstmac_fid_ep,
						   ep_fid);

	/* A little arbitrary... */
	if (ep_priv->int_tx_pool.enabled == false) {
		return -FI_EOPBADSTATE;
	}

	switch (ep->fid.fclass) {
	case FI_CLASS_EP:
		break;
	case FI_CLASS_TX_CTX:
		break;
	default:
    SSTMAC_INFO(FI_LOG_EP_CTRL, "Invalid EP type\n");
		return -FI_EINVAL;
	}
#endif
	/* We can queue TXs indefinitely, so just return the default size. */
  return SSTMAC_TX_SIZE_DEFAULT;
}

__attribute__((unused))
DIRECT_FN STATIC extern "C" int sstmac_tx_context(struct fid_ep *ep, int index,
				     struct fi_tx_attr *attr,
				     struct fid_ep **tx_ep, void *context)
{
  return -FI_ENOSYS;
}

__attribute__((unused))
DIRECT_FN STATIC extern "C" int sstmac_rx_context(struct fid_ep *ep, int index,
				     struct fi_rx_attr *attr,
				     struct fid_ep **rx_ep, void *context)
{
	return -FI_ENOSYS;
}

