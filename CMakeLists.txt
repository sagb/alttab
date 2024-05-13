cmake_minimum_required(VERSION 3.10)
project(alttab VERSION 1.7.1)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
enable_language(C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XFT REQUIRED xft)
pkg_check_modules(XRENDER REQUIRED xrender)
pkg_check_modules(XRANDR REQUIRED xrandr)
pkg_check_modules(LIBPNG REQUIRED libpng)
pkg_check_modules(XPM REQUIRED xpm)

pkg_check_modules(FTS QUIET libfts)
if(NOT FTS_FOUND)
  pkg_check_modules(FTS QUIET musl-fts)
endif()
if(FTS_FOUND)
  message(STATUS "Using ${FTS_PREFIX} for fts functions.")
  include_directories(${FTS_INCLUDE_DIRS})
  link_libraries(${FTS_LIBRARIES})
else()
  message(STATUS "libfts or musl-fts not found. Assuming that fts functions are included in libc.")
endif()

find_path(UTHASH_INCLUDE_DIRS NAMES uthash.h)
if(NOT UTHASH_INCLUDE_DIRS)
  message(FATAL_ERROR "Cannot find uthash.h, please install uthash")
endif()

include_directories(${X11_INCLUDE_DIRS} ${XFT_INCLUDE_DIRS} ${XRENDER_INCLUDE_DIRS} ${XRANDR_INCLUDE_DIRS} ${LIBPNG_INCLUDE_DIRS} ${XPM_INCLUDE_DIRS} ${UTHASH_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/src)

set(PACKAGE "alttab")
set(PACKAGE_STRING "${PACKAGE} ${PROJECT_VERSION}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
configure_file(src/config.h.in src/config.h)

enable_testing()

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(test)

link_libraries(${X11_LIBRARIES} ${XFT_LIBRARIES} ${XRENDER_LIBRARIES} ${XRANDR_LIBRARIES} ${LIBPNG_LIBRARIES} ${XPM_LIBRARIES} ${FTS_LIBRARIES})

install(TARGETS alttab DESTINATION ${CMAKE_INSTALL_BINDIR})
