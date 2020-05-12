/*
 * Copyright (c) 2004, 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2004, 2011-2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2005, 2006, 2007 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2005 PathScale, Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
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

#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <infiniband/ofa_verbs.h>
#include <string.h>
#include <infiniband/verbs.h>

#include <sprockit/errors.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sstmac/software/api/api.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>
#include <sumi/sim_transport.h>

class IBVMessage : public sumi::Message {
 public:
  IBVMessage(ibv_event_type tp): type_(tp) {}
  ibv_event_type get_event_type() {return type_;}
private:
  ibv_event_type type_;
};

class IBVTransport : public sumi::SimTransport {

 public:
  SST_ELI_REGISTER_DERIVED(
    API,
    IBVTransport,
    "macro",
    "ibv",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the infiniband transport API")
  public:
    IBVTransport(SST::Params& params,
                 sstmac::sw::App* parent,
                 SST::Component* comp) :
      sumi::SimTransport(params, parent, comp),
      inited_(false)
  {
  }

  void init() override {
    sumi::SimTransport::init();
    inited_ = true;
  }

  bool inited() const {
    return inited_;
  }

 private:
  bool inited_;
};

IBVTransport* sstmac_ibv()
{
  sstmac::sw::Thread* t = sstmac::sw::OperatingSystem::currentThread();
  IBVTransport* tp = t->getApi<IBVTransport>("ibv");
  if (!tp->inited())
    tp->init();
  return tp;
}


/**
 * ibv_rate_to_mult - Convert the IB rate enum to a multiple of the
 * base rate of 2.5 Gbit/sec.  For example, IBV_RATE_5_GBPS will be
 * converted to 2, since 5 Gbit/sec is 2 * 2.5 Gbit/sec.
 * @rate: rate to convert.
 */
int ibv_rate_to_mult(enum ibv_rate rate)  {
  spkt_abort_printf("unimplemented: ibv_rate_to_mult()");
  return 0;
}

/**
 * mult_to_ibv_rate - Convert a multiple of 2.5 Gbit/sec to an IB rate enum.
 * @mult: multiple to convert.
 */
enum ibv_rate mult_to_ibv_rate(int mult)  {
  spkt_abort_printf("unimplemented: ibv_rate mult_to_ibv_rate()");
  return IBV_RATE_5_GBPS;
}

/**
 * ibv_rate_to_mbps - Convert the IB rate enum to Mbit/sec.
 * For example, IBV_RATE_5_GBPS will return the value 5000.
 * @rate: rate to convert.
 */
int ibv_rate_to_mbps(enum ibv_rate rate)  {
  spkt_abort_printf("unimplemented: ibv_rate_to_mbps()");
  return 0;
}

/**
 * ibv_get_device_list - Get list of IB devices currently available
 * @num_devices: optional.  if non-NULL, set to the number of devices
 * returned in the array.
 *
 * Return a NULL-terminated array of IB devices.  The array can be
 * released with ibv_free_device_list().
 */
struct ibv_device **ibv_get_device_list(int *num_devices) {
  spkt_abort_printf("unimplemented: ibv_get_device_list()");
  return nullptr;
}

/**
 * ibv_free_device_list - Free list from ibv_get_device_list()
 *
 * Free an array of devices returned from ibv_get_device_list().  Once
 * the array is freed, pointers to devices that were not opened with
 * ibv_open_device() are no longer valid.  Client code must open all
 * devices it intends to use before calling ibv_free_device_list().
 */
void ibv_free_device_list(struct ibv_device **list) {
  spkt_abort_printf("unimplemented: ibv_free_device_list()");
}

/**
 * ibv_get_device_name - Return kernel device name
 */
const char *ibv_get_device_name(struct ibv_device *device) {
  spkt_abort_printf("unimplemented: ibv_get_device_name()");
  return nullptr;
}

/**
 * ibv_get_device_guid - Return device's node GUID
 */
uint64_t ibv_get_device_guid(struct ibv_device *device) {
  spkt_abort_printf("unimplemented: ibv_get_device_guid()");
  return 0;
}

/**
 * ibv_open_device - Initialize device for use
 */
struct ibv_context *ibv_open_device(struct ibv_device *device) {
  spkt_abort_printf("unimplemented: ibv_open_device()");
  return nullptr;
}

/**
 * ibv_close_device - Release device
 */
int ibv_close_device(struct ibv_context *context) {
  spkt_abort_printf("unimplemented: ibv_close_device()");
  return 0;
}

/**
 * ibv_get_async_event - Get next async event
 * @event: Pointer to use to return async event
 *
 * All async events returned by ibv_get_async_event() must eventually
 * be acknowledged with ibv_ack_async_event().
 */
int ibv_get_async_event(struct ibv_context *context,
            struct ibv_async_event *event) {
  spkt_abort_printf("unimplemented: ibv_get_async_event()");
  return 0;
}

/**
 * ibv_ack_async_event - Acknowledge an async event
 * @event: Event to be acknowledged.
 *
 * All async events which are returned by ibv_get_async_event() must
 * be acknowledged.  To avoid races, destroying an object (CQ, SRQ or
 * QP) will wait for all affiliated events to be acknowledged, so
 * there should be a one-to-one correspondence between acks and
 * successful gets.
 */
void ibv_ack_async_event(struct ibv_async_event *event) {
  spkt_abort_printf("unimplemented: ibv_get_async_event()");
}

/**
 * ibv_query_device - Get device properties
 */
int ibv_query_device(struct ibv_context *context,
             struct ibv_device_attr *device_attr) {
  spkt_abort_printf("unimplemented: ibv_query_device()");
  return 0;
}

/**
 * ibv_query_gid - Get a GID table entry
 */
int ibv_query_gid(struct ibv_context *context, uint8_t port_num,
          int index, union ibv_gid *gid) {
  spkt_abort_printf("unimplemented: ibv_query_gid()");
  return 0;
}

/**
 * ibv_query_pkey - Get a P_Key table entry
 */
int ibv_query_pkey(struct ibv_context *context, uint8_t port_num,
           int index, uint16_t *pkey) {
  spkt_abort_printf("unimplemented: ibv_query_pkey()");
  return 0;
}

/**
 * ibv_alloc_pd - Allocate a protection domain
 */
struct ibv_pd *ibv_alloc_pd(struct ibv_context *context) {
  struct ibv_pd *pd;
  pd = new struct ibv_pd;
  pd->context = context;
  return pd;
}

/**
 * ibv_dealloc_pd - Free a protection domain
 */
int ibv_dealloc_pd(struct ibv_pd *pd) {
  spkt_abort_printf("unimplemented: ibv_dealloc_pd()");
  return 0;
}

/**
 * ibv_reg_mr - Register a memory region
 */
struct ibv_mr *ibv_reg_mr(struct ibv_pd *pd, void *addr,
              size_t length, int access) {
  spkt_abort_printf("unimplemented: ibv_reg_mr()");
  return nullptr;
}

/**
 * ibv_rereg_mr - Re-Register a memory region
 */
int ibv_rereg_mr(struct ibv_mr *mr, int flags,
		 struct ibv_pd *pd, void *addr,
         size_t length, int access) {
  spkt_abort_printf("unimplemented: ibv_rereg_mr()");
  return 0;
}

/**
 * ibv_dereg_mr - Deregister a memory region
 */
int ibv_dereg_mr(struct ibv_mr *mr) {
  spkt_abort_printf("unimplemented: ibv_dereg_mr()");
  return 0;
}

/**
 * ibv_create_comp_channel - Create a completion event channel
 */
struct ibv_comp_channel *ibv_create_comp_channel(struct ibv_context *context) {
  ibv_comp_channel *ch;
  ch = new ibv_comp_channel;

  ch->context = context;
  //ch->fd      = resp.fd;
  ch->refcnt  = 0;

  return ch;
}

/**
 * ibv_destroy_comp_channel - Destroy a completion event channel
 */
int ibv_destroy_comp_channel(struct ibv_comp_channel *channel) {
  spkt_abort_printf("unimplemented: ibv_destroy_comp_channel()");
  return 0;
}

/**
 * ibv_create_cq - Create a completion queue
 * @context - Context CQ will be attached to
 * @cqe - Minimum number of entries required for CQ
 * @cq_context - Consumer-supplied context returned for completion events
 * @channel - Completion channel where completion events will be queued.
 *     May be NULL if completion events will not be used.
 * @comp_vector - Completion vector used to signal completion events.
 *     Must be >= 0 and < context->num_comp_vectors.
 */
struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe,
			     void *cq_context,
			     struct ibv_comp_channel *channel,
                 int comp_vector) {
  //ignore cqe (size of queue) for now
  //cq_context is NULL in example code, ignore for now
  ibv_cq* cq = new ibv_cq;
  IBVTransport* tp = sstmac_ibv();
  cq->handle = tp->allocateCqId();
  cq->channel = channel;
  return cq;
}

/**
 * ibv_resize_cq - Modifies the capacity of the CQ.
 * @cq: The CQ to resize.
 * @cqe: The minimum size of the CQ.
 *
 * Users can examine the cq structure to determine the actual CQ size.
 */
int ibv_resize_cq(struct ibv_cq *cq, int cqe) {
  spkt_abort_printf("unimplemented: ibv_resize_cq()");
  return 0;
}

/**
 * ibv_destroy_cq - Destroy a completion queue
 */
int ibv_destroy_cq(struct ibv_cq *cq) {
  spkt_abort_printf("unimplemented: ibv_destroy_cq()");
  return 0;
}

/**
 * ibv_get_cq_event - Read next CQ event
 * @channel: Channel to get next event from.
 * @cq: Used to return pointer to CQ.
 * @cq_context: Used to return consumer-supplied CQ context.
 *
 * All completion events returned by ibv_get_cq_event() must
 * eventually be acknowledged with ibv_ack_cq_events().
 */
int ibv_get_cq_event(struct ibv_comp_channel *channel,
             struct ibv_cq **cq, void **cq_context) {
  spkt_abort_printf("unimplemented: ibv_get_cq_event()");
  return 0;
}

/**
 * ibv_ack_cq_events - Acknowledge CQ completion events
 * @cq: CQ to acknowledge events for
 * @nevents: Number of events to acknowledge.
 *
 * All completion events which are returned by ibv_get_cq_event() must
 * be acknowledged.  To avoid races, ibv_destroy_cq() will wait for
 * all completion events to be acknowledged, so there should be a
 * one-to-one correspondence between acks and successful gets.  An
 * application may accumulate multiple completion events and
 * acknowledge them in a single call to ibv_ack_cq_events() by passing
 * the number of events to ack in @nevents.
 */
void ibv_ack_cq_events(struct ibv_cq *cq, unsigned int nevents) {
  spkt_abort_printf("unimplemented: ibv_ack_cq_events()");
}

/**
 * ibv_create_srq - Creates a SRQ associated with the specified protection
 *   domain.
 * @pd: The protection domain associated with the SRQ.
 * @srq_init_attr: A list of initial attributes required to create the SRQ.
 *
 * srq_attr->max_wr and srq_attr->max_sge are read the determine the
 * requested size of the SRQ, and set to the actual values allocated
 * on return.  If ibv_create_srq() succeeds, then max_wr and max_sge
 * will always be at least as large as the requested values.
 */
struct ibv_srq *ibv_create_srq(struct ibv_pd *pd,
                   struct ibv_srq_init_attr *srq_init_attr) {
  spkt_abort_printf("unimplemented: ibv_create_srq()");
  return nullptr;
}

/**
 * ibv_modify_srq - Modifies the attributes for the specified SRQ.
 * @srq: The SRQ to modify.
 * @srq_attr: On input, specifies the SRQ attributes to modify.  On output,
 *   the current values of selected SRQ attributes are returned.
 * @srq_attr_mask: A bit-mask used to specify which attributes of the SRQ
 *   are being modified.
 *
 * The mask may contain IBV_SRQ_MAX_WR to resize the SRQ and/or
 * IBV_SRQ_LIMIT to set the SRQ's limit and request notification when
 * the number of receives queued drops below the limit.
 */
int ibv_modify_srq(struct ibv_srq *srq,
		   struct ibv_srq_attr *srq_attr,
           int srq_attr_mask) {
  spkt_abort_printf("unimplemented: ibv_modify_srq()");
  return 0;
}

/**
 * ibv_query_srq - Returns the attribute list and current values for the
 *   specified SRQ.
 * @srq: The SRQ to query.
 * @srq_attr: The attributes of the specified SRQ.
 */
int ibv_query_srq(struct ibv_srq *srq, struct ibv_srq_attr *srq_attr) {
  spkt_abort_printf("unimplemented: ibv_query_srq()");
  return 0;
}

/**
 * ibv_destroy_srq - Destroys the specified SRQ.
 * @srq: The SRQ to destroy.
 */
int ibv_destroy_srq(struct ibv_srq *srq) {
  spkt_abort_printf("unimplemented: ibv_destroy_srq()");
  return 0;
}

/**
 * ibv_create_qp - Create a queue pair.
 */
struct ibv_qp *ibv_create_qp(struct ibv_pd *pd,
                 struct ibv_qp_init_attr *qp_init_attr) {
  spkt_abort_printf("unimplemented: ibv_create_qp()");
  return nullptr;
}

/**
 * ibv_modify_qp - Modify a queue pair.
 */
int ibv_modify_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
          int attr_mask) {
  spkt_abort_printf("unimplemented: ibv_modify_qp()");
  return 0;
}

/**
 * ibv_query_qp - Returns the attribute list and current values for the
 *   specified QP.
 * @qp: The QP to query.
 * @attr: The attributes of the specified QP.
 * @attr_mask: A bit-mask used to select specific attributes to query.
 * @init_attr: Additional attributes of the selected QP.
 *
 * The qp_attr_mask may be used to limit the query to gathering only the
 * selected attributes.
 */
int ibv_query_qp(struct ibv_qp *qp, struct ibv_qp_attr *attr,
		 int attr_mask,
         struct ibv_qp_init_attr *init_attr) {
  spkt_abort_printf("unimplemented: ibv_query_qp()");
  return 0;
}

/**
 * ibv_destroy_qp - Destroy a queue pair.
 */
int ibv_destroy_qp(struct ibv_qp *qp) {
  spkt_abort_printf("unimplemented: ibv_destroy_qp()");
  return 0;
}

/**
 * ibv_create_ah - Create an address handle.
 */
struct ibv_ah *ibv_create_ah(struct ibv_pd *pd, struct ibv_ah_attr *attr) {
  spkt_abort_printf("unimplemented: ibv_create_ah()");
  return nullptr;
}

/**
 * ibv_init_ah_from_wc - Initializes address handle attributes from a
 *   work completion.
 * @context: Device context on which the received message arrived.
 * @port_num: Port on which the received message arrived.
 * @wc: Work completion associated with the received message.
 * @grh: References the received global route header.  This parameter is
 *   ignored unless the work completion indicates that the GRH is valid.
 * @ah_attr: Returned attributes that can be used when creating an address
 *   handle for replying to the message.
 */
int ibv_init_ah_from_wc(struct ibv_context *context, uint8_t port_num,
			struct ibv_wc *wc, struct ibv_grh *grh,
            struct ibv_ah_attr *ah_attr) {
  spkt_abort_printf("unimplemented: ibv_init_ah_from_wc()");
  return 0;
}

/**
 * ibv_create_ah_from_wc - Creates an address handle associated with the
 *   sender of the specified work completion.
 * @pd: The protection domain associated with the address handle.
 * @wc: Work completion information associated with a received message.
 * @grh: References the received global route header.  This parameter is
 *   ignored unless the work completion indicates that the GRH is valid.
 * @port_num: The outbound port number to associate with the address.
 *
 * The address handle is used to reference a local or global destination
 * in all UD QP post sends.
 */
struct ibv_ah *ibv_create_ah_from_wc(struct ibv_pd *pd, struct ibv_wc *wc,
                     struct ibv_grh *grh, uint8_t port_num) {
  spkt_abort_printf("unimplemented: ibv_create_ah_from_wc()");
  return nullptr;
}

/**
 * ibv_destroy_ah - Destroy an address handle.
 */
int ibv_destroy_ah(struct ibv_ah *ah) {
  spkt_abort_printf("unimplemented: ibv_destroy_ah()");
  return 0;
}

/**
 * ibv_attach_mcast - Attaches the specified QP to a multicast group.
 * @qp: QP to attach to the multicast group.  The QP must be a UD QP.
 * @gid: Multicast group GID.
 * @lid: Multicast group LID in host byte order.
 *
 * In order to route multicast packets correctly, subnet
 * administration must have created the multicast group and configured
 * the fabric appropriately.  The port associated with the specified
 * QP must also be a member of the multicast group.
 */
int ibv_attach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid) {
  spkt_abort_printf("unimplemented: ibv_attach_mcast()");
  return 0;
}

/**
 * ibv_detach_mcast - Detaches the specified QP from a multicast group.
 * @qp: QP to detach from the multicast group.
 * @gid: Multicast group GID.
 * @lid: Multicast group LID in host byte order.
 */
int ibv_detach_mcast(struct ibv_qp *qp, const union ibv_gid *gid, uint16_t lid) {
  spkt_abort_printf("unimplemented: ibv_detach_mcast()");
  return 0;
}

/**
 * ibv_fork_init - Prepare data structures so that fork() may be used
 * safely.  If this function is not called or returns a non-zero
 * status, then libibverbs data structures are not fork()-safe and the
 * effect of an application calling fork() is undefined.
 */
int ibv_fork_init(void) {
  spkt_abort_printf("unimplemented: ibv_fork_init()");
  return 0;
}

/**
 * ibv_node_type_str - Return string describing node_type enum value
 */
const char *ibv_node_type_str(enum ibv_node_type node_type) {
  spkt_abort_printf("unimplemented: ibv_node_type_str())");
  return nullptr;
}

/**
 * ibv_port_state_str - Return string describing port_state enum value
 */
const char *ibv_port_state_str(enum ibv_port_state port_state) {
  spkt_abort_printf("unimplemented: ibv_port_state_str())");
  return nullptr;
}

/**
 * ibv_event_type_str - Return string describing event_type enum value
 */
const char *ibv_event_type_str(enum ibv_event_type event) {
  spkt_abort_printf("unimplemented: ibv_event_type_str())");
  return nullptr;
}
