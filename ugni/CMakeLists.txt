include(GNUInstallDirs)

add_sstmac_library(ugni
  libpmi.cc
  libugni.cc
  libibverbs.c
)

install(FILES
  gni_pub.h
  hugetlbfs.h
  pmi.h
  pmi_cray.h
  udreg_pub.h
  verbs.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS ugni LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

configure_file(cray-gni-headers.pc.in ${CMAKE_BINARY_DIR}/cray-gni-headers.pc @ONLY)
configure_file(cray-ugni.pc.in ${CMAKE_BINARY_DIR}/cray-ugni.pc @ONLY)
configure_file(env.sh.in ${CMAKE_BINARY_DIR}/env.sh @ONLY)

install(FILES
  ${CMAKE_BINARY_DIR}/cray-ugni.pc
  ${CMAKE_BINARY_DIR}/cray-gni-headers.pc
  ${CMAKE_BINARY_DIR}/env.sh
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
