# Install script for directory: /Users/jpkenny/src/sst-transports/libfabric

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/jpkenny/install/sst-transports")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_abi.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_atom.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_enosys.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_file.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_hook.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_indexer.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_iov.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_list.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_bitmask.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/shared/ofi_str.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_lock.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_mem.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_osd.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_proto.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_recvwin.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_rbuf.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_shm.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_signal.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_epoll.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_tree.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_util.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_atomic.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_mr.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_net.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_perf.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_coll.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/fasthash.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rbtree.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/uthash.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/ofi_prov.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/providers/fi_log.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/providers/fi_prov.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/rdma" TYPE FILE FILES
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fabric.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_atomic.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_cm.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_collective.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_domain.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_eq.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_rma.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_endpoint.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_errno.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_tagged.h"
    "/Users/jpkenny/src/sst-transports/libfabric/include/rdma/fi_trigger.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/jpkenny/src/sst-transports/libfabric/libfabric.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libfabric.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libfabric.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/local/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libfabric.dylib")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

