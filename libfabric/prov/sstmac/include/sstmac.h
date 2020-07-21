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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef compiler_barrier
#define compiler_barrier() asm volatile ("" ::: "memory")
#endif

#define SSTMAC_MAX_MSG_IOV_LIMIT 1
#define SSTMAC_MAX_RMA_IOV_LIMIT 1
#define SSTMAC_MAX_ATOMIC_IOV_LIMIT 1


#define SSTMAC_SUPPORTED_FLAGS (FI_NUMERICHOST | FI_SOURCE)

#define SSTMAC_DEFAULT_FLAGS (0)

#define SSTMAC_DOM_CAPS \
  (FI_LOCAL_COMM | FI_REMOTE_COMM | FI_SHARED_AV)

#define SSTMAC_EP_PRIMARY_CAPS                                               \
	(FI_MSG | FI_RMA | FI_TAGGED | FI_ATOMICS |                            \
	 FI_DIRECTED_RECV | FI_READ | FI_NAMED_RX_CTX |                        \
	 FI_WRITE | FI_SEND | FI_RECV | FI_REMOTE_READ | FI_REMOTE_WRITE)

#define SSTMAC_EP_SEC_CAPS (FI_MULTI_RECV | FI_TRIGGER | FI_FENCE)

#define SSTMAC_EP_OP_FLAGS	(FI_INJECT | FI_MULTI_RECV | FI_COMPLETION | \
				 FI_INJECT_COMPLETE | FI_TRANSMIT_COMPLETE | \
				 FI_DELIVERY_COMPLETE)

#define SSTMAC_SENDMSG_FLAGS	(FI_REMOTE_CQ_DATA | FI_COMPLETION | \
				 FI_MORE | FI_INJECT | FI_INJECT_COMPLETE | \
				 FI_TRANSMIT_COMPLETE | FI_FENCE | FI_TRIGGER)
#define SSTMAC_RECVMSG_FLAGS	(FI_COMPLETION | FI_MORE | FI_MULTI_RECV)
#define SSTMAC_TRECVMSG_FLAGS \
	(SSTMAC_RECVMSG_FLAGS | FI_CLAIM | FI_PEEK | FI_DISCARD)

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

#define SSTMAC_RMA_COMPLETION_FLAGS	(FI_RMA | FI_READ | FI_WRITE)
#define SSTMAC_AMO_COMPLETION_FLAGS	(FI_ATOMIC | FI_READ | FI_WRITE)

#define SSTMAC_TX_SIZE_DEFAULT 100
#define SSTMAC_RX_SIZE_DEFAULT 1000

#define SSTMAC_RX_CTX_MAX_BITS	8
#define SSTMAC_SEP_MAX_CNT	(1 << (SSTMAC_RX_CTX_MAX_BITS - 1))

#define SSTMAC_MAX_MSG_SIZE (1<<31)
#define SSTMAC_INJECT_SIZE 64
#define SSTMAC_MAX_INJECT_SIZE 64

#define SSTMAC_FAB_MODES	0

#define SSTMAC_FAB_MODES_CLEAR (FI_MSG_PREFIX | FI_ASYNC_IOV)


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

struct sstmac_fid_ep {
	struct fid_ep ep_fid;
	enum fi_ep_type type;
	struct sstmac_fid_domain *domain;
	uint64_t op_flags;
	uint64_t caps;
  uint16_t rx_id;
  fi_addr_t src_addr;
  fi_addr_t dest_addr;
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
	struct sstmac_fid_eq *eq;
  int qos;
};


struct sstmac_fid_trx {
  sstmac_fid_ep ep;
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

struct sstmac_fid_eq {
  struct fid_eq eq_fid;
  struct sstmac_fid_fabric* fabric;
  struct fid_wait* wait;
  struct fi_eq_attr attr;
};

struct sstmac_progress_queue;
struct sstmac_fid_cq {
  struct fid_cq cq_fid;
  struct sstmac_fid_domain *domain;
  int id; //the sumi CQ id allocated to this
  enum fi_cq_format format;
  size_t entry_size;
  struct fid_wait *wait;
  sstmac_progress_queue* queue;
};

struct sstmac_fid_srx {
  struct fid_ep ep_fid;
  sstmac_fid_domain* domain;
};

#define GET_SSTMAC_ADDR_CQ(addr)    ((addr) & 0xFFFF)
#define GET_SSTMAC_ADDR_RANK(addr)  (((addr) >> 32) & 0xFFFFFFFF)
#define PUT_SSTMAC_ADDR_RANK(rank)    ((rank) << 32)
#define PUT_SSTMAC_ADDR_CQ(cq)        (cq)

#define SSTMAC_MAX_ADDR_CHARS 7
#define SSTMAC_ADDR_FORMAT_STR "%7" PRIu64
#define SSTMAC_MAX_ADDR_LEN SSTMAC_MAX_ADDR_CHARS+1

extern const char sstmac_fab_name[];
extern const char sstmac_dom_name[];

#define warn_einval(...) \
  fprintf(stderr, "EINVAL: " __VA_ARGS__); \
  fprintf(stderr, "  %s:%d\n", __FILE__, __LINE__)


/**
 * @brief str_to_fi_addr
 * @param str a string of the node address
 * @return limits<fi_addr_t>::max() on failure, otherwise the value
 */
fi_addr_t sstmaci_str_to_fi_addr(const char* str);

/**
 * @brief sstmaci_fi_addr_to_str
 * @param dest
 * @return The number of chars that would have been written in large enough buffer (same as snprintf)
 */
int sstmaci_fi_addr_to_str(fi_addr_t, char* dest, int dest_size);

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

void _sstmac_init(void);

#ifdef FABRIC_DIRECT_ENABLED
#define DIRECT_FN __attribute__((visibility ("default")))
#define STATIC
#define EXTERN_C extern "C"
#else
#define DIRECT_FN
#define STATIC static
#define EXTERN_C
#endif

#ifdef __cplusplus
}

#include <sstmac/software/process/progress_queue.h>
#include <sumi/message.h>
#include <sstmac_sumi.hpp>

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

struct RecvQueue {

  struct Recv {
    uint32_t size;
    void* buf;
    void* context;
    Recv(uint32_t s, void* b, void* ctx) :
      size(s), buf(b), context(ctx)
    {
    }
  };

  struct TaggedRecv {
    uint32_t size;
    void* buf;
    void* context;
    uint64_t tag;
    uint64_t tag_ignore;
    TaggedRecv(uint32_t s, void* b, void* ctx,
               uint64_t t, uint64_t ti) :
      size(s), buf(b), context(ctx), tag(t), tag_ignore(ti)
    {
    }
  };

  RecvQueue(sstmac::sw::OperatingSystem* os) :
    progress(os)
  {
  }

  bool matches(FabricMessage* msg, uint64_t tag, uint64_t ignore){
    return (msg->tag() & ~ignore) == (tag & ~ignore);
  }

  std::list<Recv> recvs;
  std::list<TaggedRecv> tagged_recvs;
  std::list<FabricMessage*> unexp_recvs;
  std::list<FabricMessage*> unexp_tagged_recvs;

  sstmac::sw::SingleProgressQueue<sumi::Message> progress;

  void finishMatch(void* buf, uint32_t size, void* context, FabricMessage* fmsg);

  void matchTaggedRecv(FabricMessage* msg);

  void postRecv(uint32_t size, void* buf, uint64_t tag,
                uint64_t tag_ignore, bool tagged, void* context);

  void incoming(sumi::Message* msg);

};


#endif

#endif /* _SSTMAC_H_ */
