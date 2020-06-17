set(sstmac_sources
  prov/sstmac/src/sstmac_av.cc
  prov/sstmac/src/sstmac_cm.cc
  prov/sstmac/src/sstmac_cntr.cc
  prov/sstmac/src/sstmac_cq.cc
  prov/sstmac/src/sstmac_dom.cc
  prov/sstmac/src/sstmac_ep.cc
  prov/sstmac/src/sstmac_eq.cc
  prov/sstmac/src/sstmac_fabric.cc
  prov/sstmac/src/sstmac_mr.cc
  prov/sstmac/src/sstmac_poll.cc
  prov/sstmac/src/sstmac_sep.cc
  prov/sstmac/src/sstmac_wait.cc
)

set(sstmac_headers
	prov/sstmac/include/fi_ext_sstmac.h
  prov/sstmac/include/fi_ext_sstmac.h
  prov/sstmac/include/sstmac.h
  prov/sstmac/include/sstmac_av.h
  prov/sstmac/include/sstmac_ep.h
  prov/sstmac/include/sstmac_poll.h
  prov/sstmac/include/sstmac_wait.h
)

set(sstmac_rdma_headers
  prov/sstmac/include/rdma/fi_direct.h
  prov/sstmac/include/rdma/fi_direct_atomic.h
  prov/sstmac/include/rdma/fi_direct_atomic_def.h
  prov/sstmac/include/rdma/fi_direct_cm.h
  prov/sstmac/include/rdma/fi_direct_domain.h
  prov/sstmac/include/rdma/fi_direct_endpoint.h
  prov/sstmac/include/rdma/fi_direct_eq.h
  prov/sstmac/include/rdma/fi_direct_rma.h
  prov/sstmac/include/rdma/fi_direct_tagged.h
  prov/sstmac/include/rdma/fi_direct_trigger.h
)

set(common_sources
  src/log.c
  src/common.c
  src/rbtree.c
  src/indexer.c
  src/perf.c
  src/mem.c
  src/iov.c
  src/shared/ofi_str.c
  src/abi_1_0.c
  src/unix/osd.c
  src/enosys.c
  src/fabric.c
  src/fasthash.c
  src/var.c
  src/fi_tostr.c
  src/tree.c
	prov/util/src/util_atomic.c
	prov/util/src/util_attr.c
	prov/util/src/util_av.c
	prov/util/src/util_cq.c
	prov/util/src/util_cntr.c
	prov/util/src/util_domain.c
	prov/util/src/util_ep.c
	prov/util/src/util_pep.c
	prov/util/src/util_eq.c
	prov/util/src/util_fabric.c
	prov/util/src/util_main.c
	prov/util/src/util_poll.c
	prov/util/src/util_wait.c
	prov/util/src/util_buf.c
	prov/util/src/util_mr_map.c
	prov/util/src/util_ns.c
	prov/util/src/util_shm.c
	prov/util/src/util_mem_monitor.c
	prov/util/src/util_mem_hooks.c
	prov/util/src/util_mr_cache.c
	prov/util/src/util_coll.c
)

if (APPLE)
  list(APPEND common_sources 
    src/unix/osd.c
    include/osx/osd.h
    include/unix/osd.h)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND common_sources
    src/unix/osd.c
    src/linux/osd.c
    src/linux/rdpmc.c
    include/linux/rdpmc.h
    include/linux/osd.h
    include/unix/osd.h)
#common_srcs += src/linux/rdpmc.c
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  list(APPEND common_sources
    src/unix/osd.c
    include/freebsd/osd.h
    include/unix/osd.h)
endif()

set(libfabric_sources
  prov/hook/src/hook_cm.c
  prov/hook/src/hook_cq.c
  prov/hook/src/hook_xfer.c
  prov/hook/src/hook_ep.c
  prov/hook/src/hook_cntr.c
  prov/hook/src/hook_av.c
  prov/hook/src/hook.c
  prov/hook/src/hook_wait.c
  prov/hook/src/hook_domain.c
  prov/hook/src/hook_eq.c
)

set(libfabric_headers
	include/ofi.h
	include/ofi_abi.h
	include/ofi_atom.h
	include/ofi_enosys.h
	include/ofi_file.h
	include/ofi_hook.h
	include/ofi_indexer.h
	include/ofi_iov.h
	include/ofi_list.h
	include/ofi_bitmask.h
	include/shared/ofi_str.h
	include/ofi_lock.h
	include/ofi_mem.h
	include/ofi_osd.h
	include/ofi_proto.h
	include/ofi_recvwin.h
	include/ofi_rbuf.h
	include/ofi_shm.h
	include/ofi_signal.h
	include/ofi_epoll.h
	include/ofi_tree.h
	include/ofi_util.h
	include/ofi_atomic.h
	include/ofi_mr.h
	include/ofi_net.h
	include/ofi_perf.h
	include/ofi_coll.h
	include/fasthash.h
	include/rbtree.h
	include/uthash.h
	include/ofi_prov.h
	include/rdma/providers/fi_log.h
	include/rdma/providers/fi_prov.h
)

set(rdmainclude_headers
	prov/sstmac/include/fi_ext_sstmac.h
	include/rdma/fabric.h
	include/rdma/fi_atomic.h
	include/rdma/fi_cm.h
	include/rdma/fi_collective.h
	include/rdma/fi_domain.h
	include/rdma/fi_eq.h
	include/rdma/fi_rma.h
	include/rdma/fi_endpoint.h
	include/rdma/fi_errno.h
	include/rdma/fi_tagged.h
	include/rdma/fi_trigger.h
)

set(nodist_rdmainclude_headers
	prov/sstmac/include/rdma/fi_direct_domain.h
	prov/sstmac/include/rdma/fi_direct_endpoint.h
	prov/sstmac/include/rdma/fi_direct_tagged.h
	prov/sstmac/include/rdma/fi_direct_rma.h
	prov/sstmac/include/rdma/fi_direct_atomic_def.h
	prov/sstmac/include/rdma/fi_direct_atomic.h
	prov/sstmac/include/rdma/fi_direct_cm.h
	prov/sstmac/include/rdma/fi_direct_eq.h
	prov/sstmac/include/rdma/fi_direct_trigger.h
	prov/sstmac/include/rdma/fi_direct.h
)

add_library(fabric SHARED
  ${sstmac_sources}
  ${sstmac_headers}
  ${common_sources} 
  ${libfabric_sources}
  ${libfabric_headers}
  ${rdmainclude_headers}
  ${nodist_rdmainclude_headers}
  ${sstmac_rdma_headers}
)
target_link_libraries(fabric PRIVATE SST::SSTMacro)

include(GNUInstallDirs)
target_compile_definitions(fabric PRIVATE
	-D_GNU_SOURCE -D__USE_XOPEN2K8 
	-DSYSCONFDIR=${CMAKE_INSTALL_SYSCONFDIR}
	-DRDMADIR
	-DPROVDLDIR=\"${CMAKE_INSTALL_LIBDIR}/libfabric\"
)
target_include_directories(fabric PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(fabric PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/prov/sstmac
  ${CMAKE_CURRENT_SOURCE_DIR}/prov/sstmac/include
  ${CMAKE_CURRENT_SOURCE_DIR}/prov/hook/include
)

set(BUILD_ID)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(ENABLE_DEBUG On)
endif()

include(cmake/tests.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

install(FILES ${libfabric_headers}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(TARGETS fabric LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
