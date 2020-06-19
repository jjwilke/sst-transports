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

#ifndef _SSTMAC_H_
#define _SSTMAC_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <rdma/fabric.h>
#include <rdma/fi_atomic.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_eq.h>
#include <rdma/fi_errno.h>
#include <rdma/providers/fi_prov.h>
#include <rdma/fi_rma.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_trigger.h>

#include <ofi.h>
#include <ofi_atomic.h>
#include <ofi_enosys.h>
#include <ofi_rbuf.h>
#include <ofi_list.h>
#include <ofi_file.h>

#include "fi_ext_sstmac.h"


#ifdef __cplusplus
extern "C" {
#endif

struct sstmac_mem_handle_t {
};

/*
 * useful macros
 */
#ifndef FLOOR
#define FLOOR(a, b) ((long long)(a) - (((long long)(a)) % (b)))
#endif

#ifndef CEILING
#define CEILING(a, b) ((long long)(a) <= 0LL ? 0 : (FLOOR((a)-1, b) + (b)))
#endif

#ifndef compiler_barrier
#define compiler_barrier() asm volatile ("" ::: "memory")
#endif

#define SSTMAC_MAX_MSG_IOV_LIMIT 8
#define SSTMAC_MAX_RMA_IOV_LIMIT 1
#define SSTMAC_MAX_ATOMIC_IOV_LIMIT 1
#define SSTMAC_ADDR_CACHE_SIZE 5

/*
 * GNI GET alignment
 */

#define SSTMAC_READ_ALIGN		4
#define SSTMAC_READ_ALIGN_MASK	(SSTMAC_READ_ALIGN - 1)

/*
 * GNI IOV GET alignment
 *
 * We always pull 4byte chucks for unaligned GETs. To prevent stomping on
 * someone else's head or tail data, each segment must be four bytes
 * (i.e. SSTMAC_READ_ALIGN bytes).
 *
 * Note: "* 2" for head and tail
 */
#define SSTMAC_INT_TX_BUF_SZ (SSTMAC_MAX_MSG_IOV_LIMIT * SSTMAC_READ_ALIGN * 2)

/*
 * Flags
 * The 64-bit flag field is used as follows:
 * 1-grow up    common (usable with multiple operations)
 * 59-grow down operation specific (used for single call/class)
 * 60 - 63      provider specific
 */

#define SSTMAC_SUPPRESS_COMPLETION	(1ULL << 60)	/* TX only flag */

#define SSTMAC_RMA_RDMA			(1ULL << 61)	/* RMA only flag */
#define SSTMAC_RMA_INDIRECT		(1ULL << 62)	/* RMA only flag */
#define SSTMAC_RMA_CHAINED		(1ULL << 63)	/* RMA only flag */

#define SSTMAC_MSG_RENDEZVOUS		(1ULL << 61)	/* MSG only flag */
#define SSTMAC_MSG_GET_TAIL		(1ULL << 62)	/* MSG only flag */

/*
 * SSTMAC provider supported flags for fi_getinfo argument for now, needs
 * refining (see fi_getinfo.3 man page)
 */
#define SSTMAC_SUPPORTED_FLAGS (FI_NUMERICHOST | FI_SOURCE)

#define SSTMAC_DEFAULT_FLAGS (0)

/*
 * SSTMAC provider will try to support the fabric interface capabilities (see
 * fi_getinfo.3 man page)
 * for RDM and MSG (future) endpoint types.
 */

/*
 * See capabilities section in fi_getinfo.3.
 */

#define SSTMAC_DOM_CAPS (FI_REMOTE_COMM)

/* Primary capabilities.  Each must be explicitly requested (unless the full
 * set is requested by setting input hints->caps to NULL). */
#define SSTMAC_EP_PRIMARY_CAPS                                               \
	(FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS |                            \
	 FI_DIRECTED_RECV | FI_READ | FI_NAMED_RX_CTX |                        \
	 FI_WRITE | FI_SEND | FI_RECV | FI_REMOTE_READ | FI_REMOTE_WRITE)

/* No overhead secondary capabilities.  These can be silently enabled by the
 * provider. */
#define SSTMAC_EP_SEC_CAPS (FI_MULTI_RECV | FI_TRIGGER | FI_FENCE)

/* FULL set of capabilities for the provider.  */
#define SSTMAC_EP_CAPS_FULL (SSTMAC_EP_PRIMARY_CAPS | SSTMAC_EP_SEC_CAPS)

/*
 * see Operations flags in fi_endpoint.3
 */
#define SSTMAC_EP_OP_FLAGS	(FI_INJECT | FI_MULTI_RECV | FI_COMPLETION | \
				 FI_INJECT_COMPLETE | FI_TRANSMIT_COMPLETE | \
				 FI_DELIVERY_COMPLETE)

/*
 * Valid msg transaction input flags.  See fi_msg.3.
 */
#define SSTMAC_SENDMSG_FLAGS	(FI_REMOTE_CQ_DATA | FI_COMPLETION | \
				 FI_MORE | FI_INJECT | FI_INJECT_COMPLETE | \
				 FI_TRANSMIT_COMPLETE | FI_FENCE | FI_TRIGGER)
#define SSTMAC_RECVMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_MULTI_RECV)
#define SSTMAC_TRECVMSG_FLAGS \
	(SSTMAC_RECVMSG_FLAGS | FI_CLAIM | FI_PEEK | FI_DISCARD)

/*
 * Valid rma transaction input flags.  See fi_rma.3.
 */
#define SSTMAC_WRITEMSG_FLAGS	(FI_REMOTE_CQ_DATA | FI_COMPLETION | \
				 FI_MORE | FI_INJECT | FI_INJECT_COMPLETE | \
				 FI_TRANSMIT_COMPLETE | FI_FENCE | FI_TRIGGER)
#define SSTMAC_READMSG_FLAGS	(FI_COMPLETION | FI_MORE | \
				 FI_FENCE | FI_TRIGGER)
#define SSTMAC_ATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_INJECT | \
				 FI_FENCE | FI_TRIGGER)
#define SSTMAC_FATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_FENCE | \
				 FI_TRIGGER)
#define SSTMAC_CATOMICMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_FENCE | \
				 FI_TRIGGER)

/*
 * Valid completion event flags.  See fi_cq.3.
 */
#define SSTMAC_RMA_COMPLETION_FLAGS	(FI_RMA | FI_READ | FI_WRITE)
#define SSTMAC_AMO_COMPLETION_FLAGS	(FI_ATOMIC | FI_READ | FI_WRITE)

/*
 * GNI provider fabric default values
 */
#define SSTMAC_TX_SIZE_DEFAULT	500
#define SSTMAC_RX_SIZE_DEFAULT	500
/*
 * based on the max number of fma descriptors without fma sharing
 */
#define SSTMAC_RX_CTX_MAX_BITS	8
#define SSTMAC_SEP_MAX_CNT	(1 << (SSTMAC_RX_CTX_MAX_BITS - 1))

/*
 * if this has to be changed, check sstmac_getinfo, etc.
 */
#define SSTMAC_MAX_MSG_SIZE ((0x1ULL << 32) - 1)
#define SSTMAC_CACHELINE_SIZE (64)
#define SSTMAC_INJECT_SIZE SSTMAC_CACHELINE_SIZE

/*
 * SSTMAC provider will require the following fabric interface modes (see
 * fi_getinfo.3 man page)
 */
#define SSTMAC_FAB_MODES	0

/*
 * fabric modes that SSTMAC provider doesn't need
 */
#define SSTMAC_FAB_MODES_CLEAR (FI_MSG_PREFIX | FI_ASYNC_IOV)

/**
 * sstmac_address struct
 *
 * @note - SSTMAC address format - used for fi_send/fi_recv, etc.
 *         These values are passed to GNI_EpBind
 *
 * @var device_addr     physical NIC address of the remote peer
 * @var cdm_id          user supplied id of the remote instance
 */
struct sstmac_address {
	uint32_t device_addr;
	uint32_t cdm_id;
};

/*
 * macro for testing whether a sstmac_address value is FI_ADDR_UNSPEC
 */

#define SSTMAC_ADDR_UNSPEC(var) (((var).device_addr == -1) && \
				((var).cdm_id == -1))
/*
 * macro testing for sstmac_address equality
 */

#define SSTMAC_ADDR_EQUAL(a, b) (((a).device_addr == (b).device_addr) && \
				((a).cdm_id == (b).cdm_id))

#define SSTMAC_CREATE_CDM_ID	0

#define SSTMAC_EPN_TYPE_UNBOUND	(1 << 0)
#define SSTMAC_EPN_TYPE_BOUND	(1 << 1)
#define SSTMAC_EPN_TYPE_SEP	(1 << 2)

struct sstmac_ep_name {
	struct sstmac_address sstmac_addr;
	struct {
		uint32_t name_type : 8;
		uint32_t cm_nic_cdm_id : 24;
		uint32_t cookie;
	};
	struct {
		uint32_t rx_ctx_cnt : 8;
		uint32_t key_offset : 12;
		uint32_t unused1 : 12;
		uint32_t unused2;
	};
	uint64_t reserved[3];
};

/* AV address string revision. */
#define SSTMAC_AV_STR_ADDR_VERSION  1

/*
 * 52 is the number of characters printed out in sstmac_av_straddr.
 *  1 is for the null terminator
 */
#define SSTMAC_AV_MAX_STR_ADDR_LEN  (52 + 1)

/*
 * 15 is the number of characters for the device addr.
 *  1 is for the null terminator
 */
#define SSTMAC_AV_MIN_STR_ADDR_LEN  (15 + 1)

/*
 * 69 is the number of characters for the printable portion of the address
 *  1 is for the null terminator
 */
#define SSTMAC_FI_ADDR_STR_LEN (69 + 1)

/*
 * enum for blocking/non-blocking progress
 */
enum sstmac_progress_type {
	SSTMAC_PRG_BLOCKING,
	SSTMAC_PRG_NON_BLOCKING
};


struct sstmac_fid_tport;

/*
 * simple struct for sstmac fabric, may add more stuff here later
 */
struct sstmac_fid_fabric {
  struct fid_fabric fab_fid;
  //this will actually be an SST/macro transport object
  struct sstmac_fid_tport* tport;
};


extern struct fi_ops_cm sstmac_ep_msg_ops_cm;
extern struct fi_ops_cm sstmac_ep_ops_cm;

/*
 * Our domains are very simple because we have nothing complicated
 * with memory registration and we don't have to worry about
 * progress modes
 */
struct sstmac_fid_domain {
	struct fid_domain domain_fid;
  struct sstmac_fid_fabric *fabric;
  uint32_t addr_format;
};

struct sstmac_fid_pep {
	struct fid_pep pep_fid;
	struct sstmac_fid_fabric *fabric;
	struct fi_info *info;
	struct sstmac_fid_eq *eq;
	struct sstmac_ep_name src_addr;
	fastlock_t lock;
	int listen_fd;
	int backlog;
	int bound;
	size_t cm_data_size;
  //struct sstmac_reference ref_cnt;
};

#define SSTMAC_CQS_PER_EP		8

struct sstmac_fid_ep_ops_en {
	uint32_t msg_recv_allowed: 1;
	uint32_t msg_send_allowed: 1;
	uint32_t rma_read_allowed: 1;
	uint32_t rma_write_allowed: 1;
	uint32_t tagged_recv_allowed: 1;
	uint32_t tagged_send_allowed: 1;
	uint32_t atomic_read_allowed: 1;
	uint32_t atomic_write_allowed: 1;
};

#define SSTMAC_INT_TX_POOL_SIZE 128
#define SSTMAC_INT_TX_POOL_COUNT 256

struct sstmac_int_tx_buf {
	struct slist_entry e;
	uint8_t *buf;
	struct sstmac_fid_mem_desc *md;
};

typedef int sstmac_return_t;

struct sstmac_int_tx_ptrs {
	struct slist_entry e;
	void *sl_ptr;
	void *buf_ptr;
	struct sstmac_fid_mem_desc *md;
};

struct sstmac_int_tx_pool {
	bool enabled;
	int nbufs;
	fastlock_t lock;
	struct slist sl;
	struct slist bl;
};

struct sstmac_addr_cache_entry {
	fi_addr_t addr;
	struct sstmac_vc *vc;
};

enum sstmac_conn_state {
	SSTMAC_EP_UNCONNECTED,
	SSTMAC_EP_CONNECTING,
	SSTMAC_EP_CONNECTED,
	SSTMAC_EP_SHUTDOWN
};

struct sstmac_fid_ep {
	struct fid_ep ep_fid;
	enum fi_ep_type type;
	struct sstmac_fid_domain *domain;
	uint64_t op_flags;
	uint64_t caps;
	uint32_t use_tag_hlist;
	struct sstmac_fid_cq *send_cq;
	struct sstmac_fid_cq *recv_cq;
	struct sstmac_fid_cntr *send_cntr;
	struct sstmac_fid_cntr *recv_cntr;
	struct sstmac_fid_cntr *write_cntr;
	struct sstmac_fid_cntr *read_cntr;
	struct sstmac_fid_cntr *rwrite_cntr;
	struct sstmac_fid_cntr *rread_cntr;
	struct sstmac_fid_av *av;
	struct sstmac_fid_stx *stx_ctx;
	struct sstmac_cm_nic *cm_nic;
	struct sstmac_nic *nic;

	/* pointer to tag matching engine */
	int (*progress_fn)(struct sstmac_fid_ep *, enum sstmac_progress_type);
	/* RX specific progress fn */
	int (*rx_progress_fn)(struct sstmac_fid_ep *, sstmac_return_t *rc);
	struct sstmac_xpmem_handle *xpmem_hndl;
	bool tx_enabled;
	bool rx_enabled;
	bool shared_tx;
	bool requires_lock;
	struct sstmac_auth_key *auth_key;
	int last_cached;
	struct sstmac_addr_cache_entry addr_cache[SSTMAC_ADDR_CACHE_SIZE];
	int send_selective_completion;
	int recv_selective_completion;
	int min_multi_recv;
	/* note this free list will be initialized for thread safe */
  //struct sstmac_freelist fr_freelist;
	struct sstmac_int_tx_pool int_tx_pool;
  //struct sstmac_reference ref_cnt;
	struct sstmac_fid_ep_ops_en ep_ops;

	struct fi_info *info;
	struct fi_ep_attr ep_attr;
	struct sstmac_ep_name src_addr;

	/* FI_EP_MSG specific. */
	struct sstmac_vc *vc;
	int conn_fd;
	int conn_state;
	struct sstmac_ep_name dest_addr;
	struct sstmac_fid_eq *eq;
};

struct sstmac_fid_sep {
	struct fid_ep ep_fid;
	enum fi_ep_type type;
	struct fid_domain *domain;
	struct fi_info *info;
	uint64_t caps;
	uint32_t cdm_id_base;
	struct fid_ep **ep_table;
	struct fid_ep **tx_ep_table;
	struct fid_ep **rx_ep_table;
	bool *enabled;
	struct sstmac_cm_nic *cm_nic;
	struct sstmac_fid_av *av;
	struct sstmac_ep_name my_name;
	fastlock_t sep_lock;
  //struct sstmac_reference ref_cnt;
};

struct sstmac_fid_trx {
	struct fid_ep ep_fid;
	struct sstmac_fid_ep *ep;
	struct sstmac_fid_sep *sep;
	uint64_t op_flags;
	uint64_t caps;
	int index;
  //struct sstmac_reference ref_cnt;
};

struct sstmac_fid_stx {
	struct fid_stx stx_fid;
	struct sstmac_fid_domain *domain;
	struct sstmac_nic *nic;
	struct sstmac_auth_key *auth_key;
  //struct sstmac_reference ref_cnt;
};

struct sstmac_fid_av {
  fid_av av_fid;
  sstmac_fid_domain* domain;
};

enum sstmac_fab_req_type {
	SSTMAC_FAB_RQ_SEND,
	SSTMAC_FAB_RQ_SENDV,
	SSTMAC_FAB_RQ_TSEND,
	SSTMAC_FAB_RQ_TSENDV,
	SSTMAC_FAB_RQ_RDMA_WRITE,
	SSTMAC_FAB_RQ_RDMA_READ,
	SSTMAC_FAB_RQ_RECV,
	SSTMAC_FAB_RQ_RECVV,
	SSTMAC_FAB_RQ_TRECV,
	SSTMAC_FAB_RQ_TRECVV,
	SSTMAC_FAB_RQ_MRECV,
	SSTMAC_FAB_RQ_AMO,
	SSTMAC_FAB_RQ_FAMO,
	SSTMAC_FAB_RQ_CAMO,
	SSTMAC_FAB_RQ_END_NON_NATIVE,
  SSTMAC_FAB_RQ_START_NATIV,
  SSTMAC_FAB_RQ_NAMO_AX,
  SSTMAC_FAB_RQ_NAMO_AX_S,
  SSTMAC_FAB_RQ_NAMO_FAX,
  SSTMAC_FAB_RQ_NAMO_FAX_S,
	SSTMAC_FAB_RQ_MAX_TYPES,
};

struct sstmac_fab_req_rma {
	uint64_t                 loc_addr;
	struct sstmac_fid_mem_desc *loc_md;
	size_t                   len;
	uint64_t                 rem_addr;
	uint64_t                 rem_mr_key;
	uint64_t                 imm;
	ofi_atomic32_t           outstanding_txds;
	sstmac_return_t             status;
	struct slist_entry       sle;
};

struct sstmac_fab_req_msg {
  //struct sstmac_tag_list_element tle;

	struct send_info_t {
		uint64_t	 send_addr;
		size_t		 send_len;
    sstmac_mem_handle_t mem_hndl;
		uint32_t	 head;
		uint32_t	 tail;
	}			     send_info[SSTMAC_MAX_MSG_IOV_LIMIT];
	struct sstmac_fid_mem_desc     *send_md[SSTMAC_MAX_MSG_IOV_LIMIT];
	size_t                       send_iov_cnt;
	uint64_t                     send_flags;
	size_t			     cum_send_len;
	struct sstmac_fab_req 	     *parent;
	size_t                       mrecv_space_left;
	uint64_t                     mrecv_buf_addr;

	struct recv_info_t {
		uint64_t	 recv_addr;
		size_t		 recv_len;
    sstmac_mem_handle_t mem_hndl;
		uint32_t	 tail_len : 2; /* If the send len is > the recv_len, we
						* need to fetch the unaligned tail into
						* the txd's int buf
						*/
		uint32_t	 head_len : 2;
	}			     recv_info[SSTMAC_MAX_MSG_IOV_LIMIT];
	struct sstmac_fid_mem_desc     *recv_md[SSTMAC_MAX_MSG_IOV_LIMIT];
	size_t			     recv_iov_cnt;
	uint64_t                     recv_flags; /* protocol, API info */
	size_t			     cum_recv_len;

	uint64_t                     tag;
	uint64_t                     ignore;
	uint64_t                     imm;
  sstmac_mem_handle_t             rma_mdh;
	uint64_t                     rma_id;
	ofi_atomic32_t               outstanding_txds;
	sstmac_return_t                 status;
};

struct sstmac_fab_req_amo {
	uint64_t                 loc_addr;
	struct sstmac_fid_mem_desc *loc_md;
	size_t                   len;
	uint64_t                 rem_addr;
	uint64_t                 rem_mr_key;
	uint64_t                 imm;
	enum fi_datatype         datatype;
	enum fi_op               op;
	uint64_t                 first_operand;
	uint64_t                 second_operand;
};

/*
 * Check for remote peer capabilities.
 * inputs:
 *   pc        - peer capabilities
 *   ops_flags - current operation flags (FI_RMA, FI_READ, etc.)
 *
 * See capabilities section in fi_getinfo.3.
 */
static inline int sstmac_rma_read_target_allowed(uint64_t pc,
					       uint64_t ops_flags)
{
	if (ops_flags & FI_RMA) {
		if (ops_flags & FI_READ) {
			if (pc & FI_RMA) {
				if (pc & FI_REMOTE_READ)
					return 1;
				if (pc & (FI_READ | FI_WRITE | FI_REMOTE_WRITE))
					return 0;
				return 1;
			}
		}
	}
	return 0;
}
static inline int sstmac_rma_write_target_allowed(uint64_t pc,
						uint64_t ops_flags)
{
	if (ops_flags & FI_RMA) {
		if (ops_flags & FI_WRITE) {
			if (pc & FI_RMA) {
				if (pc & FI_REMOTE_WRITE)
					return 1;
				if (pc & (FI_READ | FI_WRITE | FI_REMOTE_READ))
					return 0;
				return 1;
			}
		}
	}
	return 0;
}

static inline int sstmac_atomic_read_target_allowed(uint64_t pc,
						  uint64_t ops_flags)
{
	if (ops_flags & FI_ATOMICS) {
		if (ops_flags & FI_READ) {
			if (pc & FI_ATOMICS) {
				if (pc & FI_REMOTE_READ)
					return 1;
				if (pc & (FI_READ | FI_WRITE | FI_REMOTE_WRITE))
					return 0;
				return 1;
			}
		}
	}
	return 0;
}

static inline int sstmac_atomic_write_target_allowed(uint64_t pc,
						   uint64_t ops_flags)
{
	if (ops_flags & FI_ATOMICS) {
		if (ops_flags & FI_WRITE) {
			if (pc & FI_ATOMICS) {
				if (pc & FI_REMOTE_WRITE)
					return 1;
				if (pc & (FI_READ | FI_WRITE | FI_REMOTE_READ))
					return 0;
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Test if this operation is permitted based on the type of transfer
 * (encoded in the flags parameter), the endpoint capabilities and the
 * remote endpoint (peer) capabilities. Set a flag to speed up future checks.
 */

static inline int sstmac_ops_allowed(struct sstmac_fid_ep *ep,
				   uint64_t peer_caps,
				   uint64_t flags)
{
	uint64_t caps = ep->caps;

	if ((flags & FI_RMA) && (flags & FI_READ)) {
		if (OFI_UNLIKELY(!ep->ep_ops.rma_read_allowed)) {
			/* check if read initiate capabilities are allowed */
			if (caps & FI_RMA) {
				if (caps & FI_READ) {
					;
				} else if (caps & (FI_WRITE |
						   FI_REMOTE_WRITE |
						   FI_REMOTE_READ)) {
					return 0;
				}
			} else {
				return 0;
			}
			/* check if read remote capabilities are allowed */
			if (sstmac_rma_read_target_allowed(peer_caps, flags)) {
				ep->ep_ops.rma_read_allowed = 1;
				return 1;
			}
			return 0;
		}
		return 1;
	} else if ((flags & FI_RMA) && (flags & FI_WRITE)) {
		if (OFI_UNLIKELY(!ep->ep_ops.rma_write_allowed)) {
			/* check if write initiate capabilities are allowed */
			if (caps & FI_RMA) {
				if (caps & FI_WRITE) {
					;
				} else if (caps & (FI_READ |
						   FI_REMOTE_WRITE |
						   FI_REMOTE_READ)) {
					return 0;
				}
			} else {
				return 0;
			}
			/* check if write remote capabilities are allowed */
			if (sstmac_rma_write_target_allowed(peer_caps, flags)) {
				ep->ep_ops.rma_write_allowed = 1;
				return 1;
			}
			return 0;
		}
		return 1;
	} else if ((flags & FI_ATOMICS) && (flags & FI_READ)) {
		if (OFI_UNLIKELY(!ep->ep_ops.atomic_read_allowed)) {
			/* check if read initiate capabilities are allowed */
			if (caps & FI_ATOMICS) {
				if (caps & FI_READ) {
					;
				} else if (caps & (FI_WRITE |
						   FI_REMOTE_WRITE |
						   FI_REMOTE_READ)) {
					return 0;
				}
			} else {
				return 0;
			}
			/* check if read remote capabilities are allowed */
			if (sstmac_atomic_read_target_allowed(peer_caps, flags)) {
				ep->ep_ops.atomic_read_allowed = 1;
				return 1;
			}
			return 0;
		}
		return 1;
	} else if ((flags & FI_ATOMICS) && (flags & FI_WRITE)) {
		if (OFI_UNLIKELY(!ep->ep_ops.atomic_write_allowed)) {
			/* check if write initiate capabilities are allowed */
			if (caps & FI_ATOMICS) {
				if (caps & FI_WRITE) {
					;
				} else if (caps & (FI_READ |
						   FI_REMOTE_WRITE |
						   FI_REMOTE_READ)) {
					return 0;
				}
			} else {
				return 0;
			}
			/* check if write remote capabilities are allowed */
			if (sstmac_atomic_write_target_allowed(peer_caps,
							     flags)) {
				ep->ep_ops.atomic_write_allowed = 1;
				return 1;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

/**
 * Fabric request layout, there is a one to one
 * correspondence between an application's invocation of fi_send, fi_recv
 * and a sstmac fab_req.
 *
 * @var dlist	     a doubly linked list entry used to queue a request in
 * either the vc's tx_queue or work_queue.
 * @var addr	     the peer's sstmac_address associated with this request.
 * @var type	     the fabric request type
 * @var sstmac_ep      the sstmac endpoint associated with this request
 * @var user_context the user context, typically the receive buffer address for
 * a send or the send buffer address for a receive.
 * @var vc	      the virtual channel or connection edge between the sender
 * and receiver.
 * @var work_fn	     the function called by the nic progress loop to initiate
 * the fabric request.
 * @var flags	      a set of bit patterns that apply to all message types
 * @cb                optional call back to be invoked when ref cnt on this
 *                    object drops to zero
 * @ref_cnt           ref cnt for this object
 * @var iov_txds      A list of pending Rdma/CtFma GET txds.
 * @var iov_txd_cnt   The count of outstanding iov txds.
 * @var tx_failures   tx failure bits.
 * @var rma	      GNI PostRdma request
 * @var msg	      GNI SMSG request
 * @var amo	      GNI Fma request
 */
struct sstmac_fab_req {
	struct dlist_entry        dlist;
	struct sstmac_address       addr;
	enum sstmac_fab_req_type    type;
	struct sstmac_fid_ep        *sstmac_ep;
	void                      *user_context;
	struct sstmac_vc            *vc;
	int                       (*work_fn)(void *);
	uint64_t                  flags;
	void                      (*cb)(void *);
  //struct sstmac_reference     ref_cnt;

	struct slist_entry           *int_tx_buf_e;
	uint8_t                      *int_tx_buf;
	sstmac_mem_handle_t             int_tx_mdh;

	struct sstmac_tx_descriptor *iov_txds[SSTMAC_MAX_MSG_IOV_LIMIT];
	/*
	 * special value of UINT_MAX is used to indicate
	 * an unrecoverable (aka non-transient) error has occurred
	 * in one of the underlying GNI transactions
	 */
	uint32_t		  tx_failures;

	/* common to rma/amo/msg */
	union {
		struct sstmac_fab_req_rma   rma;
		struct sstmac_fab_req_msg   msg;
		struct sstmac_fab_req_amo   amo;
	};
	char inject_buf[SSTMAC_INJECT_SIZE];
};

extern int sstmac_default_user_registration_limit;
extern int sstmac_default_prov_registration_limit;
extern int sstmac_dealloc_aki_on_fabric_close;

/* This is a per-node limitation of the GNI provider. Each process
   should request only as many registrations as it intends to use
   and no more than that. */
#define SSTMAC_MAX_SCALABLE_REGISTRATIONS 4096

/*
 * work queue struct, used for handling delay ops, etc. in a generic wat
 */

struct sstmac_work_req {
	struct dlist_entry list;
	/* function to be invoked to progress this work queue req.
	   first element is pointer to data needec by the func, second
	   is a pointer to an int which will be set to 1 if progress
	   function is complete */
	int (*progress_fn)(void *, int *);
	/* data to be passed to the progress function */
	void *data;
	/* function to be invoked if this work element has completed */
	int (*completer_fn)(void *);
	/* data for completer function */
	void *completer_data;
};

/*
 * globals
 */
extern const char sstmac_fab_name[];
extern const char sstmac_dom_name[];
extern uint32_t sstmac_cdm_modes;
extern ofi_atomic32_t sstmac_id_counter;


/*
 * linked list helpers
 */

static inline void sstmac_slist_insert_tail(struct slist_entry *item,
					  struct slist *list)
{
	item->next = NULL;
	slist_insert_tail(item, list);
}

/*
 * prototypes for fi ops methods
 */
int sstmac_domain_open(struct fid_fabric *fabric, struct fi_info *info,
		     struct fid_domain **domain, void *context);

int sstmac_av_open(struct fid_domain *domain, struct fi_av_attr *attr,
		 struct fid_av **av, void *context);

int sstmac_cq_open(struct fid_domain *domain, struct fi_cq_attr *attr,
		 struct fid_cq **cq, void *context);

int sstmac_ep_open(struct fid_domain *domain, struct fi_info *info,
		   struct fid_ep **ep, void *context);

int sstmac_pep_open(struct fid_fabric *fabric,
		  struct fi_info *info, struct fid_pep **pep,
		  void *context);

int sstmac_eq_open(struct fid_fabric *fabric, struct fi_eq_attr *attr,
		 struct fid_eq **eq, void *context);

int sstmac_mr_reg(struct fid *fid, const void *buf, size_t len,
		uint64_t access, uint64_t offset, uint64_t requested_key,
		uint64_t flags, struct fid_mr **mr_o, void *context);

int sstmac_mr_regv(struct fid *fid, const struct iovec *iov,
                 size_t count, uint64_t access,
                 uint64_t offset, uint64_t requested_key,
                 uint64_t flags, struct fid_mr **mr, void *context);

int sstmac_mr_regattr(struct fid *fid, const struct fi_mr_attr *attr,
                    uint64_t flags, struct fid_mr **mr);

int sstmac_cntr_open(struct fid_domain *domain, struct fi_cntr_attr *attr,
		 struct fid_cntr **cntr, void *context);

int sstmac_sep_open(struct fid_domain *domain, struct fi_info *info,
		 struct fid_ep **sep, void *context);

int sstmac_ep_bind(fid_t fid, struct fid *bfid, uint64_t flags);

int sstmac_ep_close(fid_t fid);

/*
 * prototype for static data initialization method
 */
void _sstmac_init(void);

/* Prepend DIRECT_FN to provider specific API functions for global visibility
 * when using fabric direct.  If the API function is static use the STATIC
 * macro to bind symbols globally when compiling with fabric direct.
 */
#ifdef FABRIC_DIRECT_ENABLED
#define DIRECT_FN __attribute__((visibility ("default")))
#define STATIC
#else
#define DIRECT_FN
#define STATIC static
#endif

#ifdef __cplusplus
}

struct ErrorDeallocate {
  template <class T, class Lambda>
  ErrorDeallocate(T* t, Lambda&& l) :
    ptr(t), dealloc(std::forward<Lambda>(l))
  {
  }

  void success(){
    ptr = nullptr;
  }

  ~ErrorDeallocate(){
    if (ptr) dealloc(ptr);
  }

  void* ptr;
  std::function<void(void*)> dealloc;
};

#endif

#endif /* _SSTMAC_H_ */
