add_library(ibverbs SHARED
  libibverbs.cc
  librdmacm.cc
)
target_link_libraries(ibverbs PRIVATE SST::SSTMacro)
target_include_directories(ibverbs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GNUInstallDirs)
install(FILES rdma_cma.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES infiniband/verbs.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/infiniband)
install(TARGETS ibverbs LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
