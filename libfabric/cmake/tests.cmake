INCLUDE(CheckCXXSourceCompiles)

Check_CXX_Source_Compiles(
  "__asm__(\".symver main_, main@ABIVER_1.0\");"
  HAVE_SYMVER_SUPPORT)

Check_CXX_Source_Compiles(
"#include <stdatomic.h>
int main(){
  atomic_int a;
  atomic_init(&a, 0);
#ifdef __STDC_NO_ATOMICS__
  #error c11 atomics are not supported
#else
  return 0;
#endif
}"
  HAVE_ATOMICS)


Check_CXX_Source_Compiles(
"#include <stdatomic.h>
int main(){
  atomic_int_least32_t a;
  atomic_int_least64_t b;
  return 0;
}"
  HAVE_ATOMICS_LEAST_TYPES)

Check_CXX_Source_Compiles(
"#include <stdint.h>
int main(){
  int32_t a;
   __sync_add_and_fetch(&a, 0);
   __sync_sub_and_fetch(&a, 0);
   #if defined(__PPC__) && !defined(__PPC64__)
     #error compiler built-in atomics are not supported on PowerPC 32-bit
   #else
     return 0;
   #endif
}"
  HAVE_BUILTIN_ATOMICS)

Check_CXX_Source_Compiles(
"#include <stdint.h>
int main(){
  uint64_t d;
  uint64_t s;
  uint64_t c;
  uint64_t r;
  r = __atomic_fetch_add(&d, s, __ATOMIC_SEQ_CST);
  r = __atomic_load_8(&d, __ATOMIC_SEQ_CST);
  __atomic_exchange(&d, &s, &r, __ATOMIC_SEQ_CST);
  __atomic_compare_exchange(&d,&c,&s,0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  #if defined(__PPC__) && !defined(__PPC64__)
    #error compiler built-in memory model aware atomics are not supported on PowerPC 32-bit
  #else
    return 0;
  #endif
}"
  HAVE_BUILTIN_MM_ATOMICS)

find_path(HAVE_DLFCN_H "dlfcn.h")
find_path(HAVE_ELF_H   "elf.h")

