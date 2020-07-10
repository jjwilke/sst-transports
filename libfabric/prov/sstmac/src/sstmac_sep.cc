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
#include "sstmac_ep.h"


EXTERN_C DIRECT_FN STATIC  int sstmac_sep_bind(fid_t fid, struct fid *bfid, uint64_t flags);
static int sstmac_sep_control(fid_t fid, int command, void *arg);
static int sstmac_sep_close(fid_t fid);

static struct fi_ops sstmac_sep_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sstmac_sep_close,
  .bind = sstmac_sep_bind,
  .control = sstmac_sep_control,
  .ops_open = fi_no_ops_open
};

static int sstmac_sep_rx_ctx(struct fid_ep *sep, int index,
			   struct fi_rx_attr *attr,
			   struct fid_ep **rx_ep, void *context);
static int sstmac_sep_tx_ctx(struct fid_ep *sep, int index,
			   struct fi_tx_attr *attr,
			   struct fid_ep **tx_ep, void *context);
static struct fi_ops_ep sstmac_sep_ops = {
  .size = sizeof(struct fi_ops_ep),
  .cancel = sstmac_cancel,
  .getopt = sstmac_getopt,
  .setopt = sstmac_setopt,
  .tx_ctx = sstmac_sep_tx_ctx,
  .rx_ctx = sstmac_sep_rx_ctx,
  .rx_size_left = fi_no_rx_size_left,
  .tx_size_left = fi_no_tx_size_left,
};

DIRECT_FN STATIC ssize_t sstmac_sep_recv(struct fid_ep *ep, void *buf,
				       size_t len, void *desc,
				       fi_addr_t src_addr, void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_recvv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t src_addr,
					void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_recvmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_sep_send(struct fid_ep *ep, const void *buf,
				       size_t len, void *desc,
				       fi_addr_t dest_addr, void *context);
DIRECT_FN ssize_t sstmac_sep_sendv(struct fid_ep *ep,
				 const struct iovec *iov,
				 void **desc, size_t count,
				 fi_addr_t dest_addr,
				 void *context);
DIRECT_FN ssize_t sstmac_sep_sendmsg(struct fid_ep *ep,
				  const struct fi_msg *msg,
				  uint64_t flags);
DIRECT_FN ssize_t sstmac_sep_msg_inject(struct fid_ep *ep, const void *buf,
				      size_t len, fi_addr_t dest_addr);
DIRECT_FN ssize_t sstmac_sep_senddata(struct fid_ep *ep, const void *buf,
				    size_t len, void *desc, uint64_t data,
				    fi_addr_t dest_addr, void *context);
DIRECT_FN ssize_t
sstmac_sep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
      uint64_t data, fi_addr_t dest_addr);
static struct fi_ops_msg sstmac_sep_msg_ops = {
  .size = sizeof(struct fi_ops_msg),
  .recv = sstmac_sep_recv,
  .recvv = sstmac_sep_recvv,
  .recvmsg = sstmac_sep_recvmsg,
  .send = sstmac_sep_send,
  .sendv = sstmac_sep_sendv,
  .sendmsg = sstmac_sep_sendmsg,
  .inject = sstmac_sep_msg_inject,
  .senddata = sstmac_sep_senddata,
  .injectdata = sstmac_sep_msg_injectdata,
};

DIRECT_FN STATIC ssize_t
sstmac_sep_read(struct fid_ep *ep, void *buf, size_t len,
	      void *desc, fi_addr_t src_addr, uint64_t addr,
	      uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	       size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
         void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		 uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_sep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	       fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		  uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_sep_rma_inject(struct fid_ep *ep, const void *buf,
		    size_t len, fi_addr_t dest_addr,
		    uint64_t addr, uint64_t key);
DIRECT_FN STATIC ssize_t
sstmac_sep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		   uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		   uint64_t key, void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
			uint64_t data, fi_addr_t dest_addr, uint64_t addr,
			uint64_t key);
static struct fi_ops_rma sstmac_sep_rma_ops = {
  .size = sizeof(struct fi_ops_rma),
  .read = sstmac_sep_read,
  .readv = sstmac_sep_readv,
  .readmsg = sstmac_sep_readmsg,
  .write = sstmac_sep_write,
  .writev = sstmac_sep_writev,
  .writemsg = sstmac_sep_writemsg,
  .inject = sstmac_sep_rma_inject,
  .writedata = sstmac_sep_writedata,
  .injectdata = sstmac_sep_rma_injectdata,
};

DIRECT_FN STATIC ssize_t sstmac_sep_trecv(struct fid_ep *ep, void *buf,
					size_t len,
					void *desc, fi_addr_t src_addr,
					uint64_t tag, uint64_t ignore,
					void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_trecvv(struct fid_ep *ep,
					 const struct iovec *iov,
					 void **desc, size_t count,
					 fi_addr_t src_addr,
					 uint64_t tag, uint64_t ignore,
           void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_trecvmsg(struct fid_ep *ep,
					   const struct fi_msg_tagged *msg,
             uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_sep_tsend(struct fid_ep *ep, const void *buf,
					size_t len, void *desc,
					fi_addr_t dest_addr, uint64_t tag,
					void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_tsendv(struct fid_ep *ep,
					 const struct iovec *iov,
					 void **desc, size_t count,
					 fi_addr_t dest_addr,
					 uint64_t tag, void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_tsendmsg(struct fid_ep *ep,
					   const struct fi_msg_tagged *msg,
					   uint64_t flags);
DIRECT_FN STATIC ssize_t sstmac_sep_tinject(struct fid_ep *ep, const void *buf,
					  size_t len, fi_addr_t dest_addr,
					  uint64_t tag);
DIRECT_FN STATIC ssize_t sstmac_sep_tsenddata(struct fid_ep *ep, const void *buf,
					    size_t len, void *desc,
					    uint64_t data, fi_addr_t dest_addr,
					    uint64_t tag, void *context);
DIRECT_FN STATIC ssize_t sstmac_sep_tinjectdata(struct fid_ep *ep,
					      const void *buf,
					      size_t len, uint64_t data,
					      fi_addr_t dest_addr, uint64_t tag);
static struct fi_ops_tagged sstmac_sep_tagged_ops = {
  .size = sizeof(struct fi_ops_tagged),
  .recv = sstmac_sep_trecv,
  .recvv = sstmac_sep_trecvv,
  .recvmsg = sstmac_sep_trecvmsg,
  .send = sstmac_sep_tsend,
  .sendv = sstmac_sep_tsendv,
  .sendmsg = sstmac_sep_tsendmsg,
  .inject = sstmac_sep_tinject,
  .senddata = sstmac_sep_tsenddata,
  .injectdata = sstmac_sep_tinjectdata,
};

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
		      void *desc, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
		      size_t count, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
		       fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		       enum fi_datatype datatype, enum fi_op op);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, void *result, void *result_desc,
			 fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			 enum fi_datatype datatype, enum fi_op op,
			 void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			  void **desc, size_t count, struct fi_ioc *resultv,
			  void **result_desc, size_t result_count,
			  fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			    struct fi_ioc *resultv, void **result_desc,
			    size_t result_count, uint64_t flags);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
			  void *desc, const void *compare, void *compare_desc,
			  void *result, void *result_desc, fi_addr_t dest_addr,
			  uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			   void **desc, size_t count,
			   const struct fi_ioc *comparev,
			   void **compare_desc, size_t compare_count,
			   struct fi_ioc *resultv, void **result_desc,
			   size_t result_count, fi_addr_t dest_addr,
			   uint64_t addr, uint64_t key,
			   enum fi_datatype datatype, enum fi_op op,
			   void *context);
DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			     const struct fi_ioc *comparev, void **compare_desc,
			     size_t compare_count, struct fi_ioc *resultv,
			     void **result_desc, size_t result_count,
			     uint64_t flags);
static struct fi_ops_atomic sstmac_sep_atomic_ops = {
  .size = sizeof(struct fi_ops_atomic),
  .write = sstmac_sep_atomic_write,
  .writev = sstmac_sep_atomic_writev,
  .writemsg = sstmac_sep_atomic_writemsg,
  .inject = sstmac_sep_atomic_inject,
  .readwrite = sstmac_sep_atomic_readwrite,
  .readwritev = sstmac_sep_atomic_readwritev,
  .readwritemsg = sstmac_sep_atomic_readwritemsg,
  .compwrite = sstmac_sep_atomic_compwrite,
  .compwritev = sstmac_sep_atomic_compwritev,
  .compwritemsg = sstmac_sep_atomic_compwritemsg,
  .writevalid = sstmac_ep_atomic_valid,
  .readwritevalid = sstmac_ep_fetch_atomic_valid,
  .compwritevalid = sstmac_ep_cmp_atomic_valid,
};

/*
 * rx/tx contexts don't do any connection management,
 * nor does the underlying sstmac_fid_ep struct
 */
static struct fi_ops_cm sstmac_sep_rxtx_cm_ops = {
  .size = sizeof(struct fi_ops_cm),
  .setname = fi_no_setname,
  .getname = fi_no_getname,
  .getpeer = fi_no_getpeer,
  .connect = fi_no_connect,
  .listen = fi_no_listen,
  .accept = fi_no_accept,
  .reject = fi_no_reject,
  .shutdown = fi_no_shutdown,
  .join = fi_no_join,
};



static int sstmac_sep_tx_ctx(struct fid_ep *sep, int index,
			   struct fi_tx_attr *attr,
			   struct fid_ep **tx_ep, void *context)
{
  return -FI_ENOSYS;
}

static int sstmac_sep_rx_ctx(struct fid_ep *sep, int index,
			   struct fi_rx_attr *attr,
			   struct fid_ep **rx_ep, void *context)
{
  return -FI_ENOSYS;
}

EXTERN_C DIRECT_FN STATIC  int sstmac_sep_bind(fid_t fid, struct fid *bfid, uint64_t flags)
{
  return -FI_ENOSYS;
}

static int sstmac_sep_control(fid_t fid, int command, void *arg)
{
  return -FI_ENOSYS;
}

static int sstmac_sep_close(fid_t fid)
{
  return -FI_ENOSYS;
}

extern "C" int sstmac_sep_open(struct fid_domain *domain, struct fi_info *info,
		 struct fid_ep **sep, void *context)
{
  return -FI_ENOSYS;

}

DIRECT_FN STATIC ssize_t sstmac_sep_recv(struct fid_ep *ep, void *buf,
				       size_t len, void *desc,
				       fi_addr_t src_addr, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_recvv(struct fid_ep *ep,
					const struct iovec *iov,
					void **desc, size_t count,
					fi_addr_t src_addr,
					void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_recvmsg(struct fid_ep *ep,
					 const struct fi_msg *msg,
					 uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_send(struct fid_ep *ep, const void *buf,
				       size_t len, void *desc,
				       fi_addr_t dest_addr, void *context)
{
  return 0;
}

DIRECT_FN ssize_t sstmac_sep_sendv(struct fid_ep *ep,
				 const struct iovec *iov,
				 void **desc, size_t count,
				 fi_addr_t dest_addr,
				 void *context)
{
  return 0;
}

DIRECT_FN ssize_t sstmac_sep_sendmsg(struct fid_ep *ep,
				  const struct fi_msg *msg,
				  uint64_t flags)
{
  return 0;
}

DIRECT_FN ssize_t sstmac_sep_msg_inject(struct fid_ep *ep, const void *buf,
				      size_t len, fi_addr_t dest_addr)
{
  return 0;
}

DIRECT_FN ssize_t sstmac_sep_senddata(struct fid_ep *ep, const void *buf,
				    size_t len, void *desc, uint64_t data,
				    fi_addr_t dest_addr, void *context)
{
  return 0;
}

DIRECT_FN ssize_t
sstmac_sep_msg_injectdata(struct fid_ep *ep, const void *buf, size_t len,
			uint64_t data, fi_addr_t dest_addr)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_trecv(struct fid_ep *ep, void *buf,
					size_t len,
					void *desc, fi_addr_t src_addr,
					uint64_t tag, uint64_t ignore,
					void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_trecvv(struct fid_ep *ep,
					 const struct iovec *iov,
					 void **desc, size_t count,
					 fi_addr_t src_addr,
					 uint64_t tag, uint64_t ignore,
					 void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_trecvmsg(struct fid_ep *ep,
					   const struct fi_msg_tagged *msg,
					   uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_tsend(struct fid_ep *ep, const void *buf,
					size_t len, void *desc,
					fi_addr_t dest_addr, uint64_t tag,
					void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_tsendv(struct fid_ep *ep,
					 const struct iovec *iov,
					 void **desc, size_t count,
					 fi_addr_t dest_addr,
					 uint64_t tag, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_tsendmsg(struct fid_ep *ep,
					   const struct fi_msg_tagged *msg,
					   uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_tsenddata(struct fid_ep *ep, const void *buf,
					    size_t len, void *desc,
					    uint64_t data, fi_addr_t dest_addr,
					    uint64_t tag, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t sstmac_sep_tinjectdata(struct fid_ep *ep,
					      const void *buf,
					      size_t len, uint64_t data,
					      fi_addr_t dest_addr, uint64_t tag)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_read(struct fid_ep *ep, void *buf, size_t len,
	      void *desc, fi_addr_t src_addr, uint64_t addr,
	      uint64_t key, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_readv(struct fid_ep *ep, const struct iovec *iov, void **desc,
	       size_t count, fi_addr_t src_addr, uint64_t addr, uint64_t key,
	       void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		 uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_write(struct fid_ep *ep, const void *buf, size_t len, void *desc,
	       fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_writev(struct fid_ep *ep, const struct iovec *iov, void **desc,
		size_t count, fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg,
		  uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_rma_inject(struct fid_ep *ep, const void *buf,
		    size_t len, fi_addr_t dest_addr,
		    uint64_t addr, uint64_t key)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_writedata(struct fid_ep *ep, const void *buf, size_t len, void *desc,
		   uint64_t data, fi_addr_t dest_addr, uint64_t addr,
		   uint64_t key, void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_rma_injectdata(struct fid_ep *ep, const void *buf, size_t len,
			uint64_t data, fi_addr_t dest_addr, uint64_t addr,
			uint64_t key)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_write(struct fid_ep *ep, const void *buf, size_t count,
		      void *desc, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_writev(struct fid_ep *ep, const struct fi_ioc *iov, void **desc,
		      size_t count, fi_addr_t dest_addr, uint64_t addr,
		      uint64_t key, enum fi_datatype datatype, enum fi_op op,
		      void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_writemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_inject(struct fid_ep *ep, const void *buf, size_t count,
		       fi_addr_t dest_addr, uint64_t addr, uint64_t key,
		       enum fi_datatype datatype, enum fi_op op)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwrite(struct fid_ep *ep, const void *buf, size_t count,
			 void *desc, void *result, void *result_desc,
			 fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			 enum fi_datatype datatype, enum fi_op op,
			 void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			  void **desc, size_t count, struct fi_ioc *resultv,
			  void **result_desc, size_t result_count,
			  fi_addr_t dest_addr, uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_readwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			    struct fi_ioc *resultv, void **result_desc,
			    size_t result_count, uint64_t flags)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwrite(struct fid_ep *ep, const void *buf, size_t count,
			  void *desc, const void *compare, void *compare_desc,
			  void *result, void *result_desc, fi_addr_t dest_addr,
			  uint64_t addr, uint64_t key,
			  enum fi_datatype datatype, enum fi_op op,
			  void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwritev(struct fid_ep *ep, const struct fi_ioc *iov,
			   void **desc, size_t count,
			   const struct fi_ioc *comparev,
			   void **compare_desc, size_t compare_count,
			   struct fi_ioc *resultv, void **result_desc,
			   size_t result_count, fi_addr_t dest_addr,
			   uint64_t addr, uint64_t key,
			   enum fi_datatype datatype, enum fi_op op,
			   void *context)
{
  return 0;
}

DIRECT_FN STATIC ssize_t
sstmac_sep_atomic_compwritemsg(struct fid_ep *ep, const struct fi_msg_atomic *msg,
			     const struct fi_ioc *comparev, void **compare_desc,
			     size_t compare_count, struct fi_ioc *resultv,
			     void **result_desc, size_t result_count,
			     uint64_t flags)
{
  return 0;
}


