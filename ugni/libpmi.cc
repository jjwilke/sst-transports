#include "pmi.h"
#include <sprockit/errors.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sstmac/software/api/api.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>
#include <sumi/sim_transport.h>

MakeDebugSlot(pmi)

#define debug(tport, ...) \
  debug_printf(sprockit::dbg::pmi, "PMI Rank %d: %s", tport->rank(), \
    sprockit::sprintf(__VA_ARGS__).c_str())

extern sumi::Transport* activeTransport();

static sstmac::sw::App* current_app(){
  return sstmac::sw::OperatingSystem::currentThread()->parentApp();
}

extern "C" int PMI_Get_rank(int *ret)
{
  *ret = activeTransport()->rank();
  return PMI_SUCCESS;
}

extern "C" int PMI_Get_size(int* ret)
{
  *ret = activeTransport()->nproc();
  return PMI_SUCCESS;
}

extern "C" int
PMI2_KVS_Put(const char key[], const char value[])
{
  return PMI_SUCCESS;
}

extern "C" int
PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[], char value [], int maxvalue, int *vallen)
{
  return PMI_SUCCESS;
}

extern "C" int
PMI2_KVS_Fence(void)
{
  return PMI_SUCCESS;
}

extern "C" int
PMI2_Abort(void)
{
  sprockit::abort("unimplemented: PMI2_Abort");
  return PMI_SUCCESS;
}

extern "C" int
PMI2_Job_GetId(char jobid[], int jobid_size)
{
  auto thr = sstmac::sw::OperatingSystem::currentThread();
  ::sprintf(jobid, "%d", thr->aid());
  return PMI_SUCCESS;
}

extern "C" int 
PMI2_Init(int *spawned, int *size, int *rank, int *appnum)
{
  auto api = activeTransport();
  api->init();
  debug(api, "PMI2_Init()");
  *size = api->nproc();
  *rank = api->rank();
  *appnum = 0; //for now, we can't do multiple mpiexec launch
  *spawned = 0; //I have no idea what this is - zero!
  return PMI_SUCCESS;
}

bool pmi_finalized = false;

extern "C" int
PMI2_Finalize()
{
  pmi_finalized = true;
  auto api = activeTransport();
  debug(api, "PMI2_Finalize()");
  api->finish();
  return PMI_SUCCESS;
}

extern "C" int 
PMI_Get_nidlist_ptr(void** nidlist)
{
  *nidlist = activeTransport()->nidlist();
  return PMI_SUCCESS;
}

extern "C" int PMI_Init()
{
  activeTransport()->init();
  return PMI_SUCCESS;
}

extern "C" int PMI_Finalize()
{
  activeTransport()->finish();
  return PMI_SUCCESS;
}


extern "C" int PMI_Allgather(void *in, void *out, int len)
{
  auto tport = activeTransport();
  debug(tport, "PMI_Allgather()");
  int init_tag = tport->engine()->allocateGlobalCollectiveTag();
  tport->engine()->allgather(out, in, len, 1, init_tag, sumi::Message::default_cq);
  auto* msg = tport->engine()->blockUntilNext(sumi::Message::default_cq);
  delete msg;
  //for now, I know that any data from this will be completely ignored
  //so don't bother doing it
  return PMI_SUCCESS;
}

extern "C" int PMI_Barrier()
{
  auto api = activeTransport();
  debug(api, "PMI_Barrier()");
  int init_tag = api->engine()->allocateGlobalCollectiveTag();
  api->engine()->barrier(init_tag, sumi::Message::default_cq);
  auto* msg = api->engine()->blockUntilNext(sumi::Message::default_cq);
  delete msg;
  return PMI_SUCCESS;
}

extern "C" int 
PMI_Get_numpes_on_smp(int* num)
{
  *num = 1; //for now
  return PMI_SUCCESS;
}

