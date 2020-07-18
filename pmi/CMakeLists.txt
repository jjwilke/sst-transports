include(GNUInstallDirs)

add_library(pmi SHARED
  libpmi.cc
)

target_link_libraries(pmi PRIVATE SST::SSTMacro)
target_include_directories(pmi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
)

install(FILES
  pmi.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS pmi LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
