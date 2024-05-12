add_executable(alttab alttab.c gui.c win.c x.c rp.c util.c ewmh.c icon.c pngd.c randr.c autil.c)
include_directories(${CMAKE_BINARY_DIR}/src)

find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XFT REQUIRED xft)
pkg_check_modules(XRENDER REQUIRED xrender)
pkg_check_modules(XRANDR REQUIRED xrandr)
pkg_check_modules(LIBPNG REQUIRED libpng)
pkg_check_modules(XPM REQUIRED xpm)

target_compile_options(alttab PRIVATE ${X11_CFLAGS} ${XFT_CFLAGS} ${XRENDER_CFLAGS} ${XRANDR_CFLAGS} ${LIBPNG_CFLAGS} ${FTS_CFLAGS} ${XPM_CFLAGS} -Wall)

target_link_libraries(alttab PRIVATE ${X11_LIBRARIES} ${XFT_LIBRARIES} ${XRENDER_LIBRARIES} ${XRANDR_LIBRARIES} ${LIBPNG_LIBRARIES} ${FTS_LIBRARIES} ${XPM_LIBRARIES})
