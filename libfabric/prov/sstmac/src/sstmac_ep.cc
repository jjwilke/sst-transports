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
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);

	flags = sstmac_ep->op_flags | FI_INJECT | FI_REMOTE_CQ_DATA |
    SSTMAC_SUPPRESS_COMPLETION;

	return _sstmac_send(sstmac_ep, (uint64_t)buf, len, NULL, dest_addr,
			  NULL, flags, data, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_read(struct fid_ep *ep, void *buf, size_t len,
				      void *desc, fi_addr_t src_addr, uint64_t addr,
				      uint64_t key, void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);

  flags = sstmac_ep->op_flags | SSTMAC_RMA_READ_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_READ,
			 (uint64_t)buf, len, desc,
			 src_addr, addr, key,
			 context, flags, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	      size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
	      void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

  if (!ep || !iov || !desc || count > SSTMAC_MAX_RMA_IOV_LIMIT) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = sstmac_ep->op_flags | SSTMAC_RMA_READ_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_READ,
			 (uint64_t)iov[0].iov_base, iov[0].iov_len, desc[0],
			 src_addr, addr, key,
			 context, flags, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (!ep || !msg || !msg->msg_iov || !msg->rma_iov || !msg->desc ||
	    msg->iov_count != 1 || msg->rma_iov_count != 1 ||
	    msg->rma_iov[0].len > msg->msg_iov[0].iov_len) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = (flags & SSTMAC_READMSG_FLAGS) | SSTMAC_RMA_READ_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_READ,
			 (uint64_t)msg->msg_iov[0].iov_base,
			 msg->msg_iov[0].iov_len, msg->desc[0],
			 msg->addr, msg->rma_iov[0].addr, msg->rma_iov[0].key,
			 msg->context, flags, msg->data);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	      fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = sstmac_ep->op_flags | SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)buf, len, desc, dest_addr, addr, key,
			 context, flags, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
	       size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
	       void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

  if (!ep || !iov || !desc || count > SSTMAC_MAX_RMA_IOV_LIMIT) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = sstmac_ep->op_flags | SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)iov[0].iov_base, iov[0].iov_len, desc[0],
			 dest_addr, addr, key, context, flags, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
				uint64_t flags)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	if (!ep || !msg || !msg->msg_iov || !msg->rma_iov ||
	    msg->iov_count != 1 ||
    msg->rma_iov_count > SSTMAC_MAX_RMA_IOV_LIMIT ||
	    msg->rma_iov[0].len > msg->msg_iov[0].iov_len) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = (flags & SSTMAC_WRITEMSG_FLAGS) | SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)msg->msg_iov[0].iov_base,
			 msg->msg_iov[0].iov_len, msg->desc ? msg->desc[0] : NULL,
			 msg->addr, msg->rma_iov[0].addr, msg->rma_iov[0].key,
			 msg->context, flags, msg->data);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_ep_rma_inject(struct fid_ep *ep, const void *buf,
					    size_t len, fi_addr_t dest_addr,
					    uint64_t addr, uint64_t key)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

  flags = sstmac_ep->op_flags | FI_INJECT | SSTMAC_SUPPRESS_COMPLETION |
      SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)buf, len, NULL,
			 dest_addr, addr, key,
			 NULL, flags, 0);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		  uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		  uint64_t key, void *context)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	flags = sstmac_ep->op_flags | FI_REMOTE_CQ_DATA |
      SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)buf, len, desc,
			 dest_addr, addr, key,
			 context, flags, data);
#endif
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_ep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
		       uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		       uint64_t key)
{
#if 0
	struct sstmac_fid_ep *sstmac_ep;
	uint64_t flags;

	if (!ep) {
		return -FI_EINVAL;
	}

	sstmac_ep = container_of(ep, struct sstmac_fid_ep, ep_fid);
  assert(SSTMAC_EP_RDM_DGM_MSG(sstmac_ep->type));

	flags = sstmac_ep->op_flags | FI_INJECT | FI_REMOTE_CQ_DATA |
      SSTMAC_SUPPRESS_COMPLETION | SSTMAC_RMA_WRITE_FLAGS_DEF;

  return _sstmac_rma(sstmac_ep, SSTMAC_FAB_RQ_RDMA_WRITE,
			 (uint64_t)buf, len, NULL,
			 dest_addr, addr, key,
			 NULL, flags, data);
#endif
  return 0;
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
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_ep *ep;
	int references_held;

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);

	references_held = _sstmac_ref_put(ep);
	if (references_held)
    SSTMAC_INFO(FI_LOG_EP_CTRL, "failed to fully close ep due "
			  "to lingering references. references=%i ep=%p\n",
			  references_held, ep);
#endif
	return ret;
}

DIRECT_FN extern "C" int sstmac_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
	int ret = FI_SUCCESS;
#if 0
	struct sstmac_fid_ep  *ep;
	struct sstmac_fid_eq *eq;
	struct sstmac_fid_av  *av;
	struct sstmac_fid_cq  *cq;
	struct sstmac_fid_stx *stx;
	struct sstmac_fid_cntr *cntr;
	struct sstmac_fid_trx *trx_priv;
	struct sstmac_nic_attr nic_attr = {0};

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	switch (fid->fclass) {
	case FI_CLASS_TX_CTX:
	case FI_CLASS_RX_CTX:
		trx_priv = container_of(fid, struct sstmac_fid_trx, ep_fid);
		ep = trx_priv->ep;
		break;
	default:
		ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);
	}

	ret = ofi_ep_bind_valid(&sstmac_prov, bfid, flags);
	if (ret)
		return ret;

	/*
	 * Per fi_endpoint man page, can't bind an object
	 * to an ep after its been enabled.
	 * For scalable endpoints, the rx/tx contexts are bound to the same
	 * sstmac_ep so we allow enabling of the tx before binding the rx and
	 * vice versa.
	 */
	switch (fid->fclass) {
	case FI_CLASS_TX_CTX:
		if (ep->send_cq && ep->tx_enabled) {
			return -FI_EOPBADSTATE;
		}
		break;
	case FI_CLASS_RX_CTX:
		if (ep->recv_cq && ep->rx_enabled) {
			return -FI_EOPBADSTATE;
		}
		break;
	default:
		if ((ep->send_cq && ep->tx_enabled) ||
			(ep->recv_cq && ep->rx_enabled)) {
			return -FI_EOPBADSTATE;
		}
	}

	switch (bfid->fclass) {
	case FI_CLASS_EQ:
		eq = container_of(bfid, struct sstmac_fid_eq, eq_fid.fid);
		if (ep->domain->fabric != eq->fabric) {
			ret = -FI_EINVAL;
			break;
		}

		if (ep->eq) {
			ret = -FI_EINVAL;
			break;
		}

		ep->eq = eq;
		_sstmac_eq_poll_obj_add(eq, &ep->ep_fid.fid);
		_sstmac_ref_get(eq);

    SSTMAC_DEBUG(FI_LOG_EP_CTRL, "Bound EQ to EP: %p, %p\n", eq, ep);
		break;
	case FI_CLASS_CQ:
		cq = container_of(bfid, struct sstmac_fid_cq, cq_fid.fid);
		if (ep->domain != cq->domain) {
			ret = -FI_EINVAL;
			break;
		}
		if (flags & FI_TRANSMIT) {
			/* don't allow rebinding */
			if (ep->send_cq) {
				ret = -FI_EINVAL;
				break;
			}

			ep->send_cq = cq;
			if (flags & FI_SELECTIVE_COMPLETION) {
				ep->send_selective_completion = 1;
			}

			_sstmac_ref_get(cq);
		}
		if (flags & FI_RECV) {
			/* don't allow rebinding */
			if (ep->recv_cq) {
				ret = -FI_EINVAL;
				break;
			}

			ep->recv_cq = cq;
			if (flags & FI_SELECTIVE_COMPLETION) {
				ep->recv_selective_completion = 1;
			}

			_sstmac_ref_get(cq);
		}
		break;
	case FI_CLASS_AV:
		av = container_of(bfid, struct sstmac_fid_av, av_fid.fid);
		if (ep->domain != av->domain) {
			ret = -FI_EINVAL;
			break;
		}
		ep->av = av;
		_sstmac_ep_init_vc(ep);
		_sstmac_ref_get(ep->av);
		break;
	case FI_CLASS_CNTR:
		cntr = container_of(bfid, struct sstmac_fid_cntr, cntr_fid.fid);
		if (ep->domain != cntr->domain) {
			ret = -FI_EINVAL;
			break;
		}

		if (flags & FI_SEND) {
			/* don't allow rebinding */
			if (ep->send_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind send counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->send_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		if (flags & FI_RECV) {
			/* don't allow rebinding */
			if (ep->recv_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind recv counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->recv_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		if (flags & FI_WRITE) {
			/* don't allow rebinding */
			if (ep->write_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind write counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->write_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		if (flags & FI_READ) {
			/* don't allow rebinding */
			if (ep->read_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind read counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->read_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		if (flags & FI_REMOTE_WRITE) {
			/* don't allow rebinding */
			if (ep->rwrite_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind rwrite counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->rwrite_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		if (flags & FI_REMOTE_READ) {
			/* don't allow rebinding */
			if (ep->rread_cntr) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					  "cannot rebind rread counter (%p)\n",
					  cntr);
				ret = -FI_EINVAL;
				break;
			}
			ep->rread_cntr = cntr;
			_sstmac_ref_get(cntr);
		}

		break;

	case FI_CLASS_STX_CTX:
		stx = container_of(bfid, struct sstmac_fid_stx, stx_fid.fid);
		if (ep->domain != stx->domain) {
			ret = -FI_EINVAL;
			break;
		}

		/*
		 * can only bind an STX to an ep opened with
		 * FI_SHARED_CONTEXT ep_attr->tx_ctx_cnt and also
		 * if a nic has not been previously bound
		 */

		if (ep->shared_tx == false || ep->nic) {
			ret =  -FI_EOPBADSTATE;
			break;
		}

		/*
		 * we force allocation of a nic to make semantics
		 * match the intent fi_endpoint man page, provide
		 * a TX context (aka sstmacx nic) that can be shared
		 * explicitly amongst endpoints
		 */
		if (stx->auth_key && ep->auth_key != stx->auth_key) {
			ret = -FI_EINVAL;
			break;
		}

		if (!stx->nic) {
			nic_attr.must_alloc = true;
			nic_attr.auth_key = ep->auth_key;
			ret = sstmac_nic_alloc(ep->domain, &nic_attr,
				&stx->nic);
			if (ret != FI_SUCCESS) {
        SSTMAC_WARN(FI_LOG_EP_CTRL,
					 "_sstmac_nic_alloc call returned %d\n",
					ret);
					break;
			}
			stx->auth_key = nic_attr.auth_key;
		}

		ep->stx_ctx = stx;
		_sstmac_ref_get(ep->stx_ctx);

		ep->nic = stx->nic;
		if (ep->nic->smsg_callbacks == NULL)
			ep->nic->smsg_callbacks = sstmac_ep_smsg_callbacks;
		_sstmac_ref_get(ep->nic);
		break;

	case FI_CLASS_MR:/*TODO: got to figure this one out */
	default:
		ret = -FI_ENOSYS;
		break;
	}
#endif
	return ret;
}

static void sstmac_ep_caps(struct sstmac_fid_ep *ep_priv, uint64_t caps)
{
	if (ofi_recv_allowed(caps & ~FI_TAGGED))
		ep_priv->ep_ops.msg_recv_allowed = 1;

	if (ofi_send_allowed(caps & ~FI_TAGGED))
		ep_priv->ep_ops.msg_send_allowed = 1;

	if (ofi_recv_allowed(caps & ~FI_MSG))
		ep_priv->ep_ops.tagged_recv_allowed = 1;

	if (ofi_send_allowed(caps & ~FI_MSG))
		ep_priv->ep_ops.tagged_send_allowed = 1;

}

DIRECT_FN extern "C" int sstmac_ep_open(struct fid_domain *domain, struct fi_info *info,
			   struct fid_ep **ep, void *context)
{
	int ret = FI_SUCCESS;
#if 0
	int err_ret;
	struct sstmac_fid_domain *domain_priv;
	struct sstmac_fid_ep *ep_priv;
	struct sstmac_auth_key *auth_key;

  SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	if ((domain == NULL) || (info == NULL) || (ep == NULL) ||
	    (info->ep_attr == NULL))
		return -FI_EINVAL;

	domain_priv = container_of(domain, struct sstmac_fid_domain, domain_fid);

	if (FI_VERSION_LT(domain_priv->fabric->fab_fid.api_version,
		FI_VERSION(1, 5)) &&
		(info->ep_attr->auth_key || info->ep_attr->auth_key_size))
		return -FI_EINVAL;

#if 0
  //ignore authorization keys for now
	if (info->ep_attr->auth_key_size) {
    auth_key = SSTMAC_GET_AUTH_KEY(info->ep_attr->auth_key,
				info->ep_attr->auth_key_size,
				domain_priv->using_vmdh);
		if (!auth_key)
			return -FI_EINVAL;
	} else {
		auth_key = domain_priv->auth_key;
		assert(auth_key);
	}
#endif

  ep_priv = (sstmac_fid_ep*) calloc(1, sizeof *ep_priv);
	if (!ep_priv)
		return -FI_ENOMEM;

	/* Set up libfabric fid data. */
	ep_priv->ep_fid.fid.fclass = FI_CLASS_EP;
	ep_priv->ep_fid.fid.context = context;
	ep_priv->ep_fid.fid.ops = &sstmac_ep_fi_ops;
	ep_priv->ep_fid.ops = &sstmac_ep_ops;
	ep_priv->ep_fid.msg = &sstmac_ep_msg_ops;
	ep_priv->ep_fid.rma = &sstmac_ep_rma_ops;
	ep_priv->ep_fid.tagged = &sstmac_ep_tagged_ops;
	ep_priv->ep_fid.atomic = &sstmac_ep_atomic_ops;

  /* Init SSTMAC data. */
	ep_priv->auth_key = auth_key;
	ep_priv->type = info->ep_attr->type;
	ep_priv->domain = domain_priv;
	_sstmac_ref_init(&ep_priv->ref_cnt, 1, __ep_destruct);
  ep_priv->min_multi_recv = SSTMAC_OPT_MIN_MULTI_RECV_DEFAULT;
	fastlock_init(&ep_priv->vc_lock);
	ep_priv->progress_fn = NULL;
	ep_priv->rx_progress_fn = NULL;
	ep_priv->tx_enabled = false;
	ep_priv->rx_enabled = false;
	ep_priv->requires_lock = (domain_priv->thread_model !=
				  FI_THREAD_COMPLETION);
	ep_priv->info = fi_dupinfo(info);
	ep_priv->info->addr_format = info->addr_format;

  SSTMAC_DEBUG(FI_LOG_DEBUG, "ep(%p) is using addr_format(%s)\n", ep_priv,
		  ep_priv->info->addr_format == FI_ADDR_STR ? "FI_ADDR_STR" :
		  "FI_ADDR_SSTMAC");

	if (info->src_addr) {
		memcpy(&ep_priv->src_addr, info->src_addr,
		       sizeof(struct sstmac_ep_name));
	}

	if (info->dest_addr) {
		memcpy(&ep_priv->dest_addr, info->dest_addr,
		       sizeof(struct sstmac_ep_name));
	}

  ret = __init_tag_storages(ep_priv, SSTMAC_TAG_LIST,
				  ep_priv->type == FI_EP_MSG ? 0 : 1);
	if (ret)
		goto err_tag_init;

	ret = __fr_freelist_init(ep_priv);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			 "Error allocating sstmac_fab_req freelist (%s)",
			 fi_strerror(-ret));
		goto err_fl_init;
	}

	ep_priv->shared_tx = (info->ep_attr->tx_ctx_cnt == FI_SHARED_CONTEXT) ?
				true : false;
	/*
	 * try out XPMEM
	 */
	ret = _sstmac_xpmem_handle_create(domain_priv,
					&ep_priv->xpmem_hndl);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "_sstmac_xpmem_handl_create returned %s\n",
			  fi_strerror(-ret));
	}

	/* Initialize caps, modes, permissions, behaviors. */
  ep_priv->caps = info->caps & SSTMAC_EP_CAPS_FULL;

	if (ep_priv->info->tx_attr)
		ep_priv->op_flags = ep_priv->info->tx_attr->op_flags;
	if (ep_priv->info->rx_attr)
		ep_priv->op_flags |= ep_priv->info->rx_attr->op_flags;
  ep_priv->op_flags &= SSTMAC_EP_OP_FLAGS;

	sstmac_ep_caps(ep_priv, ep_priv->caps);

	ret = _sstmac_ep_nic_init(domain_priv, ep_priv->info, ep_priv);
	if (ret != FI_SUCCESS) {
    SSTMAC_WARN(FI_LOG_EP_CTRL,
			  "_sstmac_ep_nic_init returned %d\n",
			  ret);
		goto err_nic_init;
	}

	/* Do EP type specific initialization. */
	switch (ep_priv->type) {
	case FI_EP_DGRAM:
	case FI_EP_RDM:
		ret = _sstmac_ep_unconn_open(domain_priv, ep_priv->info, ep_priv);
		if (ret != FI_SUCCESS) {
      SSTMAC_INFO(FI_LOG_EP_CTRL,
				  "_sstmac_ep_unconn_open() failed, err: %d\n",
				  ret);
			goto err_type_init;
		}
		break;
	case FI_EP_MSG:
		ret = _sstmac_ep_msg_open(domain_priv, ep_priv->info, ep_priv);
		if (ret != FI_SUCCESS) {
      SSTMAC_INFO(FI_LOG_EP_CTRL,
				  "_sstmac_ep_msg_open() failed, err: %d\n",
				  ret);
			goto err_type_init;
		}
		break;
	default:
		ret = -FI_EINVAL;
		goto err_type_init;
	}

	_sstmac_ref_get(ep_priv->domain);

	*ep = &ep_priv->ep_fid;
	return ret;

err_type_init:
	if (ep_priv->nic)
		_sstmac_nic_free(ep_priv->nic);
	_sstmac_cm_nic_free(ep_priv->cm_nic);
err_nic_init:
	if (ep_priv->xpmem_hndl) {
		err_ret = _sstmac_xpmem_handle_destroy(ep_priv->xpmem_hndl);
		if (err_ret != FI_SUCCESS) {
      SSTMAC_WARN(FI_LOG_EP_CTRL,
				  "_sstmac_xpmem_handle_destroy returned %s\n",
				  fi_strerror(-err_ret));
		}
	}

	__fr_freelist_destroy(ep_priv);
err_fl_init:
	__destruct_tag_storages(ep_priv);
err_tag_init:
	free(ep_priv);
#endif
	return ret;
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
	int ret = FI_SUCCESS;
#if 0
	if (strcmp(ops_name, FI_SSTMAC_EP_OPS_1) == 0)
		*ops = &sstmac_ops_ep;
	else
		ret = -FI_EINVAL;
#endif
	return ret;
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
#if 0
	struct sstmac_fid_ep *sstmac_ep;

	sstmac_ep = container_of(fid, struct sstmac_fid_ep, ep_fid.fid);

	switch (optname) {
	case FI_OPT_MIN_MULTI_RECV:
		if (optlen != sizeof(size_t))
			return -FI_EINVAL;
		/*
		 * see https://github.com/ofi-cray/libfabric-cray/issues/1120
		 */
		if (*(size_t *)optval == 0UL)
			return -FI_EINVAL;
		sstmac_ep->min_multi_recv = *(size_t *)optval;
		break;
	default:
		return -FI_ENOPROTOOPT;
	}
#endif
	return 0;
}

extern "C" int sstmac_setopt(fid_t fid, int level, int optname,
				    const void *optval, size_t optlen)
{
        ssize_t ret;
#if 0
        struct sstmac_fid_ep *ep;
        struct sstmac_fid_trx *trx_ep;

        SSTMAC_TRACE(FI_LOG_EP_CTRL, "\n");

	if (!fid || !optval)
		return -FI_EINVAL;
	else if (level != FI_OPT_ENDPOINT)
		return -FI_ENOPROTOOPT;

        switch (fid->fclass) {
        case FI_CLASS_EP:
                ret = sstmac_ep_setopt(fid, level, optname, optval, optlen);
                break;

        case FI_CLASS_RX_CTX:
        case FI_CLASS_TX_CTX:
                trx_ep = container_of(fid, struct sstmac_fid_trx, ep_fid);
                ep = trx_ep->ep;
                ret = sstmac_ep_setopt(&ep->ep_fid.fid, level, optname, optval,
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

