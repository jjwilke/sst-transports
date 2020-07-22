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

sumi::Transport* sstmac_pmi()
{
  sstmac::sw::Thread* t = sstmac::sw::OperatingSystem::currentThread();
  return t->getApi<sumi::Transport>("pmi");
}

static sstmac::sw::App* current_app(){
  return sstmac::sw::OperatingSystem::currentThread()->parentApp();
}

extern "C" int PMI_Get_rank(int *ret)
{
  *ret = sstmac_pmi()->rank();
  return PMI_SUCCESS;
}

extern "C" int PMI_Get_size(int* ret)
{
  *ret = sstmac_pmi()->nproc();
  return PMI_SUCCESS;
}

static sstmac::thread_lock kvs_lock;
static std::unordered_map<std::string,
            std::unordered_map<std::string, std::string>> kvs;

extern "C" int
PMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
  kvs_lock.lock();
  kvs[kvsname][key] = value;
  kvs_lock.unlock();
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Get( const char kvsname[], const char key[], char value[], int length)
{
  kvs_lock.lock();
  auto& str = kvs[kvsname][key];
  if (length < (str.length() + 1)){
    kvs_lock.unlock();
    return PMI_ERR_INVALID_LENGTH;
  } else {
    ::strcpy(value, str.c_str());
    kvs_lock.unlock();
    return PMI_SUCCESS;
  }
}

extern "C" int
PMI2_KVS_Put(const char key[], const char value[])
{
  sprockit::abort("unimplemented error: PMI2_KVS_Put");
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
  auto api = sstmac_pmi();
  api->init();
  debug(api, "PMI2_Init()");
  *size = api->nproc();
  *rank = api->rank();
  *appnum = 0; //for now, we can't do multiple mpiexec launch
  *spawned = 0; //I have no idea what this is - zero!
  return PMI_SUCCESS;
}

typedef int PMI_BOOL;
extern "C" int PMI_Initialized( PMI_BOOL *initialized )
{
  auto api = sstmac_pmi();
  sprockit::abort("unimplemented error: PMI_Initialized");
  return PMI_SUCCESS;
}

extern "C" int PMI_Abort(int rc, const char error_msg[])
{
  sprockit::abort(error_msg);
  return PMI_SUCCESS;
}

bool pmi_finalized = false;

extern "C" int
PMI2_Finalize()
{
  pmi_finalized = true;
  auto api = sstmac_pmi();
  debug(api, "PMI2_Finalize()");
  api->finish();
  return PMI_SUCCESS;
}

extern "C" int
PMI_Get_nidlist_ptr(void** nidlist)
{
  *nidlist = sstmac_pmi()->nidlist();
  return PMI_SUCCESS;
}

extern "C" int PMI_Init(int* spawned)
{
  sstmac_pmi()->init();
  // SST does not treat processes as being spawned
  *spawned = 0;
  return PMI_SUCCESS;
}

extern "C" int PMI_Finalize()
{
  sstmac_pmi()->finish();
  return PMI_SUCCESS;
}


extern "C" int PMI_Allgather(void *in, void *out, int len)
{
  auto tport = sstmac_pmi();
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
  auto api = sstmac_pmi();
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

extern "C" int
PMI_Publish_name( const char service_name[], const char port[] )
{
  sprockit::abort("unimplemented error: PMI_Publish_name");
  return PMI_SUCCESS;
}

extern "C" int
PMI_Unpublish_name( const char service_name[] )
{
  sprockit::abort("unimplemented error: PMI_Unpublish_name");
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Get_name_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Get_key_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Get_value_length_max( int *length )
{
  *length = 256;
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Get_my_name( char kvsname[], int length )
{
  auto* api = sstmac_pmi();
  int actual_length = snprintf(kvsname, length, "app%d", api->sid().app_);
  if (actual_length > length){
    return PMI_ERR_INVALID_LENGTH;
  } else {
    return PMI_SUCCESS;
  }
}

typedef struct PMI_keyval_t
{
    char * key;
    char * val;
} PMI_keyval_t;

extern "C" int
PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizesp[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
  sprockit::abort("unimplemented error: PMI_KVS_Get_my_name");
  return PMI_SUCCESS;
}

extern "C" int
PMI_Lookup_name( const char service_name[], char port[] )
{
  sprockit::abort("unimplemented error: PMI_Lookup_name");
  return PMI_SUCCESS;
}

extern "C" int
PMI_KVS_Commit( const char kvsname[] )
{
  //currently a no-op
  return PMI_SUCCESS;
}

extern "C" int
PMI_Get_universe_size( int *size )
{
  auto* api = sstmac_pmi();
  *size = api->nproc();
  return PMI_SUCCESS;
}

extern "C" int
PMI_Get_appnum( int *appnum )
{
  auto* api = sstmac_pmi();
  *appnum = api->sid().app_;
  return PMI_SUCCESS;
}
