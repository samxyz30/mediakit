cmake_minimum_required(VERSION 3.20)
project(mediakit C CXX)

# --- flags and things
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)  # LTO on release

# --- c/c++ setup
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(NOTICE "[${PROJECT_NAME}] building for target '${CMAKE_BUILD_TYPE}'")


# --- dependencies
find_package(Qt6 REQUIRED
  COMPONENTS Widgets Multimedia MultimediaWidgets
)

qt_standard_project_setup()
message(NOTICE "[${PROJECT_NAME}] QT: ${Qt6_VERSION}")


# --- app sources
file(GLOB_RECURSE APP_SRCS CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

qt_add_executable(mediakit ${APP_SRCS})
target_link_libraries(mediakit PRIVATE
  Qt6::Widgets Qt6::Multimedia Qt6::MultimediaWidgets
)

# for win32 Release builds as GUI app to get rid of console window
if(WIN32)
  set_target_properties(mediakit PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()



# --- installation
include(GNUInstallDirs)

install(TARGETS mediakit
  BUNDLE DESTINATION .
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

qt_generate_deploy_app_script(
  TARGET mediakit
  OUTPUT_SCRIPT deploy_script
  NO_UNSUPPORTED_PLATFORM_ERROR
  DEPLOY_TOOL_OPTIONS "--plugindir bin"
)
install(SCRIPT "${deploy_script}")
