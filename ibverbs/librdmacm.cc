#include <netdb.h>
#include<rdma_cma.h>
#include <sprockit/errors.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sstmac/software/api/api.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>
#include <sumi/sim_transport.h>

using namespace sstmac::sw;

int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res)
{
  int nid = (int) OperatingSystem::currentNodeId();

  // hack so nodes 0/1 can connect
  if (nid == 0) nid = 1;
  else if (nid == 1) nid = 0;

  (*res) = new addrinfo;
  sockaddr_in* addr = new sockaddr_in;
  addr->sin_addr.s_addr = nid;
  (*res)->ai_addr = (sockaddr*) addr;
  return 0;
}

class RdmaCmMessage : public sumi::Message {
 public:
  RdmaCmMessage(rdma_cm_event_type tp): type_(tp) {}
  rdma_cm_event_type get_event_type() {return type_;}
  struct rdma_cm_id* id_;
private:
  rdma_cm_event_type type_;
};

class RdmaCmTransport : public sumi::SimTransport {

 public:
  SST_ELI_REGISTER_DERIVED(
    API,
    RdmaCmTransport,
    "macro",
    "rdma_cm",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the RDMA cm transport API")
  public:
    RdmaCmTransport(SST::Params& params,
                 sstmac::sw::App* parent,
                 SST::Component* comp) :
      sumi::SimTransport(params, parent, comp),
      inited_(false)
  {
  }

  void getEvent(struct rdma_event_channel *channel,
                 struct rdma_cm_event **event) {
    (*event) = new rdma_cm_event;
    sumi::Message *msg = blockingPoll(channel->fd);
    RdmaCmMessage *rmsg = dynamic_cast<RdmaCmMessage*>(msg);
    (*event)->event = rmsg->get_event_type();
    (*event)->id = rmsg->id_;
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

RdmaCmTransport* sstmac_rdma_cm()
{
  sstmac::sw::Thread* t = sstmac::sw::OperatingSystem::currentThread();
  RdmaCmTransport* tp = t->getApi<RdmaCmTransport>("rdma_cm");
  if (!tp->inited())
    tp->init();
  return tp;
}

/**
 * rdma_create_event_channel - Open a channel used to report communication events.
 * Description:
 *   Asynchronous events are reported to users through event channels.  Each
 *   event channel maps to a file descriptor.
 * Notes:
 *   All created event channels must be destroyed by calling
 *   rdma_destroy_event_channel.  Users should call rdma_get_cm_event to
 *   retrieve events on an event channel.
 * See also:
 *   rdma_get_cm_event, rdma_destroy_event_channel
 */
struct rdma_event_channel *rdma_create_event_channel(void)
{
  rdma_event_channel* ec = new rdma_event_channel;
  RdmaCmTransport* tp = sstmac_rdma_cm();
  ec->fd = tp->allocateCqId();
  return ec;
}

/**
 * rdma_destroy_event_channel - Close an event communication channel.
 * @channel: The communication channel to destroy.
 * Description:
 *   Release all resources associated with an event channel and closes the
 *   associated file descriptor.
 * Notes:
 *   All rdma_cm_id's associated with the event channel must be destroyed,
 *   and all returned events must be acked before calling this function.
 * See also:
 *  rdma_create_event_channel, rdma_get_cm_event, rdma_ack_cm_event
 */
void rdma_destroy_event_channel(struct rdma_event_channel *channel)
{
  spkt_abort_printf("unimplemented: rdma_destroy_event_channel()");
}

/**
 * rdma_create_id - Allocate a communication identifier.
 * @channel: The communication channel that events associated with the
 *   allocated rdma_cm_id will be reported on.
 * @id: A reference where the allocated communication identifier will be
 *   returned.
 * @context: User specified context associated with the rdma_cm_id.
 * @ps: RDMA port space.
 * Description:
 *   Creates an identifier that is used to track communication information.
 * Notes:
 *   Rdma_cm_id's are conceptually equivalent to a socket for RDMA
 *   communication.  The difference is that RDMA communication requires
 *   explicitly binding to a specified RDMA device before communication
 *   can occur, and most operations are asynchronous in nature.  Communication
 *   events on an rdma_cm_id are reported through the associated event
 *   channel.  Users must release the rdma_cm_id by calling rdma_destroy_id.
 * See also:
 *   rdma_create_event_channel, rdma_destroy_id, rdma_get_devices,
 *   rdma_bind_addr, rdma_resolve_addr, rdma_connect, rdma_listen,
 */
int rdma_create_id(struct rdma_event_channel *channel,
		   struct rdma_cm_id **id, void *context,
		   enum rdma_port_space ps)
{
  // there's a whole bunch of stuff in the rdma_cm_id struct, but I'm not
  // sure what in there we actually need to care about
  (*id) = new rdma_cm_id;
  (*id)->channel = channel;
  return 0;
}

/**
 * rdma_create_ep - Allocate a communication identifier and qp.
 * @id: A reference where the allocated communication identifier will be
 *   returned.
 * @res: Result from rdma_getaddrinfo, which specifies the source and
 *   destination addresses, plus optional routing and connection information.
 * @pd: Optional protection domain.  This parameter is ignored if qp_init_attr
 *   is NULL.
 * @qp_init_attr: Optional attributes for a QP created on the rdma_cm_id.
 * Description:
 *   Create an identifier and option QP used for communication.
 * Notes:
 *   If qp_init_attr is provided, then a queue pair will be allocated and
 *   associated with the rdma_cm_id.  If a pd is provided, the QP will be
 *   created on that PD.  Otherwise, the QP will be allocated on a default
 *   PD.
 *   The rdma_cm_id will be set to use synchronous operations (connect,
 *   listen, and get_request).  To convert to asynchronous operation, the
 *   rdma_cm_id should be migrated to a user allocated event channel.
 * See also:
 *   rdma_create_id, rdma_create_qp, rdma_migrate_id, rdma_connect,
 *   rdma_listen
 */
int rdma_create_ep(struct rdma_cm_id **id, struct rdma_addrinfo *res,
		   struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr)
{
  spkt_abort_printf("unimplemented: rdma_create_ep()");
  return 0;
}

/**
 * rdma_destroy_ep - Deallocates a communication identifier and qp.
 * @id: The communication identifer to destroy.
 * Description:
 *   Destroys the specified rdma_cm_id and any associated QP created
 *   on that id.
 * See also:
 *   rdma_create_ep
 */
void rdma_destroy_ep(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_destroy_ep()");
}

/**
 * rdma_destroy_id - Release a communication identifier.
 * @id: The communication identifier to destroy.
 * Description:
 *   Destroys the specified rdma_cm_id and cancels any outstanding
 *   asynchronous operation.
 * Notes:
 *   Users must free any associated QP with the rdma_cm_id before
 *   calling this routine and ack an related events.
 * See also:
 *   rdma_create_id, rdma_destroy_qp, rdma_ack_cm_event
 */
int rdma_destroy_id(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_destroy_id()");
  return 0;
}

/**
 * rdma_bind_addr - Bind an RDMA identifier to a source address.
 * @id: RDMA identifier.
 * @addr: Local address information.  Wildcard values are permitted.
 * Description:
 *   Associates a source address with an rdma_cm_id.  The address may be
 *   wildcarded.  If binding to a specific local address, the rdma_cm_id
 *   will also be bound to a local RDMA device.
 * Notes:
 *   Typically, this routine is called before calling rdma_listen to bind
 *   to a specific port number, but it may also be called on the active side
 *   of a connection before calling rdma_resolve_addr to bind to a specific
 *   address.
 * See also:
 *   rdma_create_id, rdma_listen, rdma_resolve_addr, rdma_create_qp
 */
int rdma_bind_addr(struct rdma_cm_id *id, struct sockaddr *addr)
{
  spkt_abort_printf("unimplemented: rdma_bind_addr()");
  return 0;
}

/**
 * rdma_resolve_addr - Resolve destination and optional source addresses.
 * @id: RDMA identifier.
 * @src_addr: Source address information.  This parameter may be NULL.
 * @dst_addr: Destination address information.
 * @timeout_ms: Time to wait for resolution to complete.
 * Description:
 *   Resolve destination and optional source addresses from IP addresses
 *   to an RDMA address.  If successful, the specified rdma_cm_id will
 *   be bound to a local device.
 * Notes:
 *   This call is used to map a given destination IP address to a usable RDMA
 *   address.  If a source address is given, the rdma_cm_id is bound to that
 *   address, the same as if rdma_bind_addr were called.  If no source
 *   address is given, and the rdma_cm_id has not yet been bound to a device,
 *   then the rdma_cm_id will be bound to a source address based on the
 *   local routing tables.  After this call, the rdma_cm_id will be bound to
 *   an RDMA device.  This call is typically made from the active side of a
 *   connection before calling rdma_resolve_route and rdma_connect.
 * See also:
 *   rdma_create_id, rdma_resolve_route, rdma_connect, rdma_create_qp,
 *   rdma_get_cm_event, rdma_bind_addr
 */
int rdma_resolve_addr(struct rdma_cm_id *id, struct sockaddr *src_addr,
		      struct sockaddr *dst_addr, int timeout_ms)
{
  sockaddr_in* addr = (sockaddr_in*) dst_addr;
  int node = addr->sin_addr.s_addr;

  RdmaCmMessage* msg = new RdmaCmMessage(RDMA_CM_EVENT_ADDR_RESOLVED);
  msg->id_ = id;
  RdmaCmTransport* api = sstmac_rdma_cm();
  int me = api->rank();
  return 0;
}

/**
 * rdma_resolve_route - Resolve the route information needed to establish a connection.
 * @id: RDMA identifier.
 * @timeout_ms: Time to wait for resolution to complete.
 * Description:
 *   Resolves an RDMA route to the destination address in order to establish
 *   a connection.  The destination address must have already been resolved
 *   by calling rdma_resolve_addr.
 * Notes:
 *   This is called on the client side of a connection after calling
 *   rdma_resolve_addr, but before calling rdma_connect.
 * See also:
 *   rdma_resolve_addr, rdma_connect, rdma_get_cm_event
 */
int rdma_resolve_route(struct rdma_cm_id *id, int timeout_ms)
{
  spkt_abort_printf("unimplemented: rdma_resolve_route()");
  return 0;
}

/**
 * rdma_create_qp - Allocate a QP.
 * @id: RDMA identifier.
 * @pd: Optional protection domain for the QP.
 * @qp_init_attr: initial QP attributes.
 * Description:
 *  Allocate a QP associated with the specified rdma_cm_id and transition it
 *  for sending and receiving.
 * Notes:
 *   The rdma_cm_id must be bound to a local RDMA device before calling this
 *   function, and the protection domain must be for that same device.
 *   QPs allocated to an rdma_cm_id are automatically transitioned by the
 *   librdmacm through their states.  After being allocated, the QP will be
 *   ready to handle posting of receives.  If the QP is unconnected, it will
 *   be ready to post sends.
 *   If pd is NULL, then the QP will be allocated using a default protection
 *   domain associated with the underlying RDMA device.
 * See also:
 *   rdma_bind_addr, rdma_resolve_addr, rdma_destroy_qp, ibv_create_qp,
 *   ibv_modify_qp
 */
int rdma_create_qp(struct rdma_cm_id *id, struct ibv_pd *pd,
		   struct ibv_qp_init_attr *qp_init_attr)
{
  spkt_abort_printf("unimplemented: rdma_create_qp()");
  return 0;
}

int rdma_create_qp_ex(struct rdma_cm_id *id,
		      struct ibv_qp_init_attr_ex *qp_init_attr)
{
  spkt_abort_printf("unimplemented: rdma_create_qp_ex()");
  return 0;
}

int rdma_create_qp_exp(struct rdma_cm_id *id,
		       struct ibv_exp_qp_init_attr *qp_init_attr)
{
  spkt_abort_printf("unimplemented: rdma_create_qp_exp()");
  return 0;
}


/**
 * rdma_destroy_qp - Deallocate a QP.
 * @id: RDMA identifier.
 * Description:
 *   Destroy a QP allocated on the rdma_cm_id.
 * Notes:
 *   Users must destroy any QP associated with an rdma_cm_id before
 *   destroying the ID.
 * See also:
 *   rdma_create_qp, rdma_destroy_id, ibv_destroy_qp
 */
void rdma_destroy_qp(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_destroy_qp()");
}

/**
 * rdma_connect - Initiate an active connection request.
 * @id: RDMA identifier.
 * @conn_param: optional connection parameters.
 * Description:
 *   For a connected rdma_cm_id, this call initiates a connection request
 *   to a remote destination.  For an unconnected rdma_cm_id, it initiates
 *   a lookup of the remote QP providing the datagram service.
 * Notes:
 *   Users must have resolved a route to the destination address
 *   by having called rdma_resolve_route before calling this routine.
 *   A user may override the default connection parameters and exchange
 *   private data as part of the connection by using the conn_param parameter.
 * See also:
 *   rdma_resolve_route, rdma_disconnect, rdma_listen, rdma_get_cm_event
 */
int rdma_connect(struct rdma_cm_id *id, struct rdma_conn_param *conn_param)
{
//  OperatingSystem* os = current_os();
//  node* nd =  os->node();
//  nic* nc = get_nic();
//  nc->send_self_event_queue(/*timestamp*/,
//                            new_callback(this, /*function pointer*/, /*msg*/));
  return 0;
}

/**
 * rdma_listen - Listen for incoming connection requests.
 * @id: RDMA identifier.
 * @backlog: backlog of incoming connection requests.
 * Description:
 *   Initiates a listen for incoming connection requests or datagram service
 *   lookup.  The listen will be restricted to the locally bound source
 *   address.
 * Notes:
 *   Users must have bound the rdma_cm_id to a local address by calling
 *   rdma_bind_addr before calling this routine.  If the rdma_cm_id is
 *   bound to a specific IP address, the listen will be restricted to that
 *   address and the associated RDMA device.  If the rdma_cm_id is bound
 *   to an RDMA port number only, the listen will occur across all RDMA
 *   devices.
 * See also:
 *   rdma_bind_addr, rdma_connect, rdma_accept, rdma_reject, rdma_get_cm_event
 */
int rdma_listen(struct rdma_cm_id *id, int backlog)
{
  spkt_abort_printf("unimplemented: rdma_listen()");
  return 0;
}

/**
 * rdma_get_request
 */
int rdma_get_request(struct rdma_cm_id *listen, struct rdma_cm_id **id)
{
  spkt_abort_printf("unimplemented: rdma_get_request()");
  return 0;
}

/**
 * rdma_accept - Called to accept a connection request.
 * @id: Connection identifier associated with the request.
 * @conn_param: Optional information needed to establish the connection.
 * Description:
 *   Called from the listening side to accept a connection or datagram
 *   service lookup request.
 * Notes:
 *   Unlike the socket accept routine, rdma_accept is not called on a
 *   listening rdma_cm_id.  Instead, after calling rdma_listen, the user
 *   waits for a connection request event to occur.  Connection request
 *   events give the user a newly created rdma_cm_id, similar to a new
 *   socket, but the rdma_cm_id is bound to a specific RDMA device.
 *   rdma_accept is called on the new rdma_cm_id.
 *   A user may override the default connection parameters and exchange
 *   private data as part of the connection by using the conn_param parameter.
 * See also:
 *   rdma_listen, rdma_reject, rdma_get_cm_event
 */
int rdma_accept(struct rdma_cm_id *id, struct rdma_conn_param *conn_param)
{
  spkt_abort_printf("unimplemented: rdma_accept()");
  return 0;
}

/**
 * rdma_reject - Called to reject a connection request.
 * @id: Connection identifier associated with the request.
 * @private_data: Optional private data to send with the reject message.
 * @private_data_len: Size of the private_data to send, in bytes.
 * Description:
 *   Called from the listening side to reject a connection or datagram
 *   service lookup request.
 * Notes:
 *   After receiving a connection request event, a user may call rdma_reject
 *   to reject the request.  If the underlying RDMA transport supports
 *   private data in the reject message, the specified data will be passed to
 *   the remote side.
 * See also:
 *   rdma_listen, rdma_accept, rdma_get_cm_event
 */
int rdma_reject(struct rdma_cm_id *id, const void *private_data,
		uint8_t private_data_len)
{
  spkt_abort_printf("unimplemented: rdma_reject()");
  return 0;
}

/**
 * rdma_notify - Notifies the librdmacm of an asynchronous event.
 * @id: RDMA identifier.
 * @event: Asynchronous event.
 * Description:
 *   Used to notify the librdmacm of asynchronous events that have occurred
 *   on a QP associated with the rdma_cm_id.
 * Notes:
 *   Asynchronous events that occur on a QP are reported through the user's
 *   device event handler.  This routine is used to notify the librdmacm of
 *   communication events.  In most cases, use of this routine is not
 *   necessary, however if connection establishment is done out of band
 *   (such as done through Infiniband), it's possible to receive data on a
 *   QP that is not yet considered connected.  This routine forces the
 *   connection into an established state in this case in order to handle
 *   the rare situation where the connection never forms on its own.
 *   Events that should be reported to the CM are: IB_EVENT_COMM_EST.
 * See also:
 *   rdma_connect, rdma_accept, rdma_listen
 */
int rdma_notify(struct rdma_cm_id *id, enum ibv_event_type event)
{
  spkt_abort_printf("unimplemented: rdma_notify()");
  return 0;
}

/**
 * rdma_disconnect - This function disconnects a connection.
 * @id: RDMA identifier.
 * Description:
 *   Disconnects a connection and transitions any associated QP to the
 *   error state.
 * See also:
 *   rdma_connect, rdma_listen, rdma_accept
 */
int rdma_disconnect(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_disconnect()");
  return 0;
}

/**
 * rdma_join_multicast - Joins a multicast group.
 * @id: Communication identifier associated with the request.
 * @addr: Multicast address identifying the group to join.
 * @context: User-defined context associated with the join request.
 * Description:
 *   Joins a multicast group and attaches an associated QP to the group.
 * Notes:
 *   Before joining a multicast group, the rdma_cm_id must be bound to
 *   an RDMA device by calling rdma_bind_addr or rdma_resolve_addr.  Use of
 *   rdma_resolve_addr requires the local routing tables to resolve the
 *   multicast address to an RDMA device.  The user must call
 *   rdma_leave_multicast to leave the multicast group and release any
 *   multicast resources.  The context is returned to the user through
 *   the private_data field in the rdma_cm_event.
 * See also:
 *   rdma_leave_multicast, rdma_bind_addr, rdma_resolve_addr, rdma_create_qp
 */
int rdma_join_multicast(struct rdma_cm_id *id, struct sockaddr *addr,
			void *context)
{
  spkt_abort_printf("unimplemented: rdma_join_multicast()");
  return 0;
}

/**
 * rdma_leave_multicast - Leaves a multicast group.
 * @id: Communication identifier associated with the request.
 * @addr: Multicast address identifying the group to leave.
 * Description:
 *   Leaves a multicast group and detaches an associated QP from the group.
 * Notes:
 *   Calling this function before a group has been fully joined results in
 *   canceling the join operation.  Users should be aware that messages
 *   received from the multicast group may stilled be queued for
 *   completion processing immediately after leaving a multicast group.
 *   Destroying an rdma_cm_id will automatically leave all multicast groups.
 * See also:
 *   rdma_join_multicast, rdma_destroy_qp
 */
int rdma_leave_multicast(struct rdma_cm_id *id, struct sockaddr *addr)
{
  spkt_abort_printf("unimplemented: rdma_leave_multicast()");
  return 0;
}

/**
 * rdma_get_cm_event - Retrieves the next pending communication event.
 * @channel: Event channel to check for events.
 * @event: Allocated information about the next communication event.
 * Description:
 *   Retrieves a communication event.  If no events are pending, by default,
 *   the call will block until an event is received.
 * Notes:
 *   The default synchronous behavior of this routine can be changed by
 *   modifying the file descriptor associated with the given channel.  All
 *   events that are reported must be acknowledged by calling rdma_ack_cm_event.
 *   Destruction of an rdma_cm_id will block until related events have been
 *   acknowledged.
 * See also:
 *   rdma_ack_cm_event, rdma_create_event_channel, rdma_event_str
 */
int rdma_get_cm_event(struct rdma_event_channel *channel,
		      struct rdma_cm_event **event)
{
  (*event) = new rdma_cm_event;
  RdmaCmTransport* tp = sstmac_rdma_cm();
  tp->getEvent(channel, event);
  return 0;
}

/**
 * rdma_ack_cm_event - Free a communication event.
 * @event: Event to be released.
 * Description:
 *   All events which are allocated by rdma_get_cm_event must be released,
 *   there should be a one-to-one correspondence between successful gets
 *   and acks.
 * See also:
 *   rdma_get_cm_event, rdma_destroy_id
 */
int rdma_ack_cm_event(struct rdma_cm_event *event)
{
  delete event;
  return 0;
}

uint16_t rdma_get_src_port(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_get_src_port()");
  return 0;
}

uint16_t rdma_get_dst_port(struct rdma_cm_id *id)
{
  spkt_abort_printf("unimplemented: rdma_get_dst_port()");
  return 0;
}

/**
 * rdma_lib_reset - Free and restart library resources
 * Description:
 *   Should be called from child process after fork to re-initialize global
 *   resources that become unavailble to child after clone().
 * Notes:
 *   To avoid memory leaks before calling this function process should try
 *   freeing all allocated librdma resources.
 */
int rdma_lib_reset(void)
{
  spkt_abort_printf("unimplemented: rdma_lib_reset()");
  return 0;
}

/**
 * rdma_get_devices - Get list of RDMA devices currently available.
 * @num_devices: If non-NULL, set to the number of devices returned.
 * Description:
 *   Return a NULL-terminated array of opened RDMA devices.  Callers can use
 *   this routine to allocate resources on specific RDMA devices that will be
 *   shared across multiple rdma_cm_id's.
 * Notes:
 *   The returned array must be released by calling rdma_free_devices.  Devices
 *   remain opened while the librdmacm is loaded.
 * See also:
 *   rdma_free_devices
 */
struct ibv_context **rdma_get_devices(int *num_devices)
{
  spkt_abort_printf("unimplemented: rdma_get_devices()");
  return nullptr;
}

/**
 * rdma_free_devices - Frees the list of devices returned by rdma_get_devices.
 * @list: List of devices returned from rdma_get_devices.
 * Description:
 *   Frees the device array returned by rdma_get_devices.
 * See also:
 *   rdma_get_devices
 */
void rdma_free_devices(struct ibv_context **list)
{
  spkt_abort_printf("unimplemented: rdma_free_devices()");
}

/**
 * rdma_event_str - Returns a string representation of an rdma cm event.
 * @event: Asynchronous event.
 * Description:
 *   Returns a string representation of an asynchronous event.
 * See also:
 *   rdma_get_cm_event
 */
const char *rdma_event_str(enum rdma_cm_event_type event)
{
  spkt_abort_printf("unimplemented: rdma_event_str()");
  return nullptr;
}

/**
 * rdma_set_option - Set options for an rdma_cm_id.
 * @id: Communication identifier to set option for.
 * @level: Protocol level of the option to set.
 * @optname: Name of the option to set.
 * @optval: Reference to the option data.
 * @optlen: The size of the %optval buffer.
 */
int rdma_set_option(struct rdma_cm_id *id, int level, int optname,
		    void *optval, size_t optlen)
{
  spkt_abort_printf("unimplemented: rdma_set_option()");
  return 0;
}

/**
 * rdma_migrate_id - Move an rdma_cm_id to a new event channel.
 * @id: Communication identifier to migrate.
 * @channel: New event channel for rdma_cm_id events.
 */
int rdma_migrate_id(struct rdma_cm_id *id, struct rdma_event_channel *channel)
{
  spkt_abort_printf("unimplemented: rdma_migrate_id()");
  return 0;
}

/**
 * rdma_getaddrinfo - RDMA address and route resolution service.
 */
int rdma_getaddrinfo(char *node, char *service,
		     struct rdma_addrinfo *hints,
		     struct rdma_addrinfo **res)
{
  spkt_abort_printf("unimplemented: rdma_getaddrinfo()");
  return 0;
}

void rdma_freeaddrinfo(struct rdma_addrinfo *res)
{
  spkt_abort_printf("unimplemented: rdma_freeaddrinfo()");
}
