if (BRLCAD_ENABLE_QT)

  find_package(Qt5 COMPONENTS Core Network Widgets)

  # Include directories needed by libpkg users
  set(PKG_EXAMPLE_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BU_INCLUDE_DIRS}
    ${PKG_INCLUDE_DIRS}
    )
  LIST(REMOVE_DUPLICATES PKG_EXAMPLE_INCLUDES)
  include_directories(${PKG_EXAMPLE_INCLUDES})

  QT5_WRAP_CPP(pkg_moc_srcs ncp.h)

  BRLCAD_ADDEXEC(qt_test_server "server.cpp;${pkg_moc_srcs}" "libbu;libpkg;Qt5::Core;Qt5::Network;Qt5::Widgets" NO_INSTALL)
  BRLCAD_ADDEXEC(qt_test_client client.cpp "libbu;libpkg" NO_INSTALL)
endif (BRLCAD_ENABLE_QT)

CMAKEFILES(
  ncp.h
  server.cpp
  client.cpp
  )
CMAKEFILES(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
