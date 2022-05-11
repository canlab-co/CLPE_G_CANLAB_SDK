cmake_minimum_required(VERSION "3.16")
project(clpe)

find_package(ament_cmake REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer_app gstreamer-app-1.0 REQUIRED IMPORTED_TARGET)

add_library(${PROJECT_NAME} SHARED
  lib/ClpeClientApi.cpp
  lib/ClpeSocket.cpp
  lib/ClpeStream.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::gstreamer_app)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

ament_export_targets(HAS_LIBRARY_TARGET ${PROJECT_NAME})
ament_package(CONFIG_EXTRAS cmake/extra_dependencies.cmake)
