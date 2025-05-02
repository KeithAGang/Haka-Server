cmake_minimum_required(VERSION 3.15)

# Project Name and Version
project(HakaServerExample VERSION 1.0 LANGUAGES CXX) # Changed project name for clarity

# Specify C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard C++ features

# Paths for Headers and Static Files in the build directory
set(CMAKE_BUILD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")
set(CMAKE_VENDOR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/_deps")
# The public directory will be copied directly to CMAKE_BINARY_DIR

# Create output directories in the build tree
file(MAKE_DIRECTORY ${CMAKE_BUILD_INCLUDE_DIR})

# Copy public folder from source tree to build tree
# This makes the public directory available relative to the executable in the build tree
file(COPY ${CMAKE_SOURCE_DIR}/public DESTINATION ${CMAKE_BINARY_DIR})

# Include CMake's FetchContent module for downloading dependencies
include(FetchContent)

# --- Download Dependencies WITHOUT Building ---
# We use FetchContent_Declare and FetchContent_Populate to download sources
# but we will manually specify include directories and link libraries.

# Set global options for FetchContent
set(FETCHCONTENT_FULLY_DISCONNECTED OFF) # Allow connecting to download
set(FETCHCONTENT_QUIET OFF) # Show download progress
set(FETCHCONTENT_UPDATES_DISCONNECTED ON) # Don't re-download if already present

# Download fmt (without building)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 9.1.0 # Using a specific tag
)

# Download Asio (standalone)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-24-0 # Using a specific tag
)

# Download yalantinglibs (contains struct_json)
FetchContent_Declare(
  yalantinglibs
  GIT_REPOSITORY https://github.com/alibaba/yalantinglibs.git
  GIT_TAG 0.4.0 # Using a specific tag
)

# Download iguana (often used with yalantinglibs, include for completeness if needed)
FetchContent_Declare(
  iguana
  GIT_REPOSITORY https://github.com/qicosmos/iguana.git
  GIT_TAG 1.0.8 # Using a specific tag
)

# Download frozen (often used with yalantinglibs, include for completeness if needed)
FetchContent_Declare(
  frozen
  GIT_REPOSITORY https://github.com/serge-sans-paille/frozen.git
  GIT_TAG master # Using master, consider a specific tag for stability
)

# Populate the content (download if needed)
FetchContent_Populate(fmt)
FetchContent_Populate(asio)
FetchContent_Populate(yalantinglibs)
FetchContent_Populate(iguana)
FetchContent_Populate(frozen)


# --- Copy Headers to Build Include Directory ---
# Copy our own headers from src/include to build/include
file(COPY ${CMAKE_SOURCE_DIR}/include/haka DESTINATION ${CMAKE_BUILD_INCLUDE_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/include/Haka.hpp DESTINATION ${CMAKE_BUILD_INCLUDE_DIR}) # Copy the main header

# Define the example executable
add_executable(haka_example
  src/main.cpp
  src/users.cpp # Corrected filename to users.cpp
)

# Make haka_example depend on copying headers

# Specify include directories for the target
target_include_directories(haka_example PRIVATE
  ${CMAKE_BUILD_INCLUDE_DIR} # Include the directory where all headers are copied
  ${CMAKE_VENDOR_INCLUDE_DIR}/asio-src/asio/include
  ${CMAKE_VENDOR_INCLUDE_DIR}/yalantinglibs-src/include
  ${CMAKE_VENDOR_INCLUDE_DIR}/iguana-src
  ${CMAKE_VENDOR_INCLUDE_DIR}/frozen-src/include
  ${CMAKE_VENDOR_INCLUDE_DIR}/fmt-src/include

  
  # You might also need to include the source directory if you have headers there
  # ${CMAKE_SOURCE_DIR}/include # If you prefer to include directly from source
)

# Specify libraries to link against
target_link_libraries(haka_example PRIVATE
  # Link against libraries needed by Asio, struct_json, filesystem
  # The exact flags might vary slightly by system and compiler

  # Removed fmt::fmt as it's not a valid target in this setup.
  # fmt is used in header-only mode via #define FMT_HEADER_ONLY.

  # For header-only libraries like Asio standalone and struct_json from ylt,
  # you often don't need explicit link libraries unless they depend on system ones.
  # However, Asio often needs system network libraries.
)

# Add platform-specific dependencies
if(WIN32)
  target_link_libraries(haka_example PRIVATE ws2_32 mswsock) # Windows networking libraries
endif()

# For std::filesystem support on some compilers/systems (like older g++),
# you might need to explicitly link the filesystem library.
# Check your compiler/system if you get linking errors related to filesystem.
# target_link_libraries(haka_example PRIVATE stdc++fs) # Example for older g++

# If fmt requires linking for certain features despite HEADER_ONLY:
# target_link_libraries(haka_example PRIVATE -lfmt) # Link against the fmt library file

# If struct_json needs linking:
# target_link_libraries(haka_example PRIVATE -lstruct_json) # Or whatever the library name is


# --- Installation Rules ---

# Install the headers from the build include directory
install(DIRECTORY ${CMAKE_BUILD_INCLUDE_DIR}/ DESTINATION include)
install(DIRECTORY ${CMAKE_VENDOR_INCLUDE_DIR}/ DESTINATION include)
# Install the public directory contents from the source tree
install(DIRECTORY ${CMAKE_SOURCE_DIR}/public/ DESTINATION public)

# Install the executable
install(TARGETS haka_example
  RUNTIME DESTINATION bin
)

# Output Messages
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Build Include Directory: ${CMAKE_BUILD_INCLUDE_DIR}")
message(STATUS "Build Vendor Include Directory: ${CMAKE_VENDOR_INCLUDE_DIR}")
message(STATUS "Build Public Directory: ${CMAKE_BINARY_DIR}/public")
