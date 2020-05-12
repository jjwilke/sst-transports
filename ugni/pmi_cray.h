#ifndef sstmac_fake_pmi_cray_h
#define sstmac_fake_pmi_cray_h

#include "pmi.h"


#ifdef __cplusplus
extern "C" {
#endif

int PMI_Get_numpes_on_smp(int* num);

int PMI2_Finalize();

int PMI2_Abort(int flag, const char msg[]);

int PMI2_KVS_Put(const char key[], const char value[]);

int PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[], char value [], int maxvalue, int *vallen);

int PMI2_KVS_Fence(void);

int PMI2_Job_GetId(char jobid[], int jobid_size);

int PMI2_Init(int *spawned, int *size, int *rank, int *appnum);

int PMI_Get_nidlist_ptr(void** nidlist);

#ifdef __cplusplus
}
#endif

#define PMI2_MAX_KEYLEN 256

#define PMI2_MAX_VALLEN 256

#define PMI2_ID_NULL -1

#endif

