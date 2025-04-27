cmake_minimum_required(VERSION 3.15)

# Project Name and Version
project(HakaServer VERSION 1.0 LANGUAGES CXX)

# Specify C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard C++ features

# Paths for Headers and Static Files
set(CMAKE_BUILD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
set(CMAKE_BUILD_PUBLIC_DIR "${CMAKE_BINARY_DIR}/public")

# Create output directories in the build tree
file(MAKE_DIRECTORY ${CMAKE_BUILD_INCLUDE_DIR})
file(MAKE_DIRECTORY ${CMAKE_BUILD_PUBLIC_DIR})

# Copy public folder from source tree to build tree
file(COPY ${CMAKE_SOURCE_DIR}/public DESTINATION ${CMAKE_BINARY_DIR})

# Include CMake's FetchContent module for downloading dependencies
include(FetchContent)

# --- Download Dependencies WITHOUT Building ---

# Set global option to prevent building where possible
set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Download fmt (without building)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 9.1.0
)

# Download Asio (standalone)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-24-0
)

# Download yalantinglibs
FetchContent_Declare(
  yalantinglibs
  GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
  GIT_TAG 0.4.0
)

# Download iguana
FetchContent_Declare(
  iguana
  GIT_REPOSITORY https://github.com/qicosmos/iguana.git
  GIT_TAG 1.0.8
)

# Download frozen
FetchContent_Declare(
  frozen
  GIT_REPOSITORY https://github.com/serge-sans-paille/frozen.git
  GIT_TAG master
)

# Download WITHOUT making available as build targets
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
endif()

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

FetchContent_GetProperties(yalantinglibs)
if(NOT yalantinglibs_POPULATED)
  FetchContent_Populate(yalantinglibs)
endif()

FetchContent_GetProperties(iguana)
if(NOT iguana_POPULATED)
  FetchContent_Populate(iguana)
endif()

FetchContent_GetProperties(frozen)
if(NOT frozen_POPULATED)
  FetchContent_Populate(frozen)
endif()

# --- Copy Headers to Build Include Directory ---

# Copy our own header
configure_file(${CMAKE_SOURCE_DIR}/include/Haka.hpp ${CMAKE_BUILD_INCLUDE_DIR}/Haka.hpp COPYONLY)

# Copy external headers with custom commands
add_custom_target(copy_headers ALL
  # Copy fmt headers
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${fmt_SOURCE_DIR}/include/fmt
    ${CMAKE_BUILD_INCLUDE_DIR}/fmt
  
  # Copy asio directory and main header
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${asio_SOURCE_DIR}/asio/include/asio
    ${CMAKE_BUILD_INCLUDE_DIR}/asio
  COMMAND ${CMAKE_COMMAND} -E copy
    ${asio_SOURCE_DIR}/asio/include/asio.hpp
    ${CMAKE_BUILD_INCLUDE_DIR}/asio.hpp
  
  # Copy ylt directory
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${yalantinglibs_SOURCE_DIR}/include/ylt
    ${CMAKE_BUILD_INCLUDE_DIR}/ylt
  
  # Copy iguana directory
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${iguana_SOURCE_DIR}/iguana
    ${CMAKE_BUILD_INCLUDE_DIR}/iguana
  
  # Copy frozen directory
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${frozen_SOURCE_DIR}/include/frozen
    ${CMAKE_BUILD_INCLUDE_DIR}/frozen
  
  # Indicate completion
  COMMENT "Copying external headers to build include directory"
)

# Define the example executable
add_executable(haka_example
  src/main.cpp
)

# Make haka_example depend on copying headers
add_dependencies(haka_example copy_headers)

# Specify include directories for the target
target_include_directories(haka_example PRIVATE
  ${CMAKE_BUILD_INCLUDE_DIR}
)

# Add platform-specific dependencies
if(WIN32)
  target_link_libraries(haka_example PRIVATE ws2_32 mswsock)
endif()

# --- Installation Rules ---

# Install the headers
install(DIRECTORY ${CMAKE_BUILD_INCLUDE_DIR}/ DESTINATION include)

# Install the public directory contents
install(DIRECTORY ${CMAKE_SOURCE_DIR}/public/ DESTINATION public)

# Install the executable
install(TARGETS haka_example
  RUNTIME DESTINATION bin
)

# Output Messages
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Build Include Directory: ${CMAKE_BUILD_INCLUDE_DIR}")
message(STATUS "Build Public Directory: ${CMAKE_BINARY_DIR}/public")