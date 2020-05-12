#ifndef sstmac_ugni_PMI_H
#define sstmac_ugni_PMI_H

#define PMI_SUCCESS 0

#ifdef __cplusplus
extern "C" {
#endif

int PMI_Get_size(int* ret);

int PMI_Get_rank(int* ret);

int PMI_Allgather(void *in, void *out, int len);

int PMI_Abort(int exit_code, const char error_msg[]);

int PMI_Barrier();

int PMI_Init();

int PMI_Finalize();




#ifdef __cplusplus
}
#endif



#endif // PMI_H
