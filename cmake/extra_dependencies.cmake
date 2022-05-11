# pkgconfig dependencies that are not find_package-able
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer_app gstreamer-app-1.0 REQUIRED IMPORTED_TARGET)
