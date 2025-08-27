cmake_minimum_required(VERSION 3.31)

# Only this generators supported for this project
set(PROJECT_SUPPORTED_GENERATORS_LIST
    "Unix Makefiles"
    "MinGW Makefiles"
)
if(NOT CMAKE_GENERATOR IN_LIST PROJECT_SUPPORTED_GENERATORS_LIST)
  message(FATAL_ERROR "Unsupported generator. Use: ${PROJECT_SUPPORTED_GENERATORS_LIST}")
endif()


# Check Crosscompiling
if(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "Crosscompiling not supported by this project")
endif()


# Find Git commit details to get project version from latest commit tag,
# becouse this project uses Git tag system for version control.
find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LATEST_TAG_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
# Extract version number
if(GIT_LATEST_TAG_STRING MATCHES "v([0-9]+\.[0-9]+\.[0-9]+)")
  set(DOMPDFUI_VERSION ${CMAKE_MATCH_1})
else()
  message(FATAL_ERROR "Can't get project version from Git. "
                      "Check latest commit tag string, so that it contains the necessary data")
endif()


# Setup project options
project(dompdfui VERSION ${DOMPDFUI_VERSION})
set(BUILD_SHARED_LIBS OFF)
set(DOMPDF_VERSION "3.1.0")
set(PHPCLI_VERSION "8.4.11")


# Downloading CPM.cmake
file(
  DOWNLOAD "https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/CPM.cmake"
  EXPECTED_HASH SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
)
set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE PATH "")
include(${CMAKE_CURRENT_SOURCE_DIR}/external/CPM.cmake)


# Find boost library
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_STATIC_RUNTIME     ON)
find_package(Boost 1.89.0 REQUIRED COMPONENTS program_options nowide process asio predef tokenizer)


# Get timestamp library
CPMAddPackage("gh:abramov7613/cmake_timestamp@1.0.3")
if (NOT cmake_timestamp_ADDED)
  message(FATAL_ERROR "Can't get cmake_timestamp library")
endif()


#[[ Get hash-library
CPMAddPackage(
  NAME md5_lib
  URL "https://github.com/stbrumme/hash-library/archive/refs/heads/master.zip"
  URL_HASH SHA256=c21cf95abaf5077ad17080bf348609feccefbfd24cf261b74efda879a6a1cf78
  DOWNLOAD_ONLY YES
)
if (NOT md5_lib_ADDED)
  message(FATAL_ERROR "Can't get hash-library library")
endif()
add_library(md5_lib STATIC "${md5_lib_SOURCE_DIR}/md5.cpp")
target_include_directories(md5_lib PUBLIC $<BUILD_INTERFACE:${md5_lib_SOURCE_DIR}>)]]


# Define embedded resources
add_executable(resource_generator embed_main.cpp)
target_compile_features(resource_generator PRIVATE cxx_std_20)
target_include_directories(resource_generator PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(resource_generator PRIVATE Boost::program_options Boost::nowide)
set(PHP_EXE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/php.exe")
set(DOMPDF_ZIP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/dompdf.zip")
add_custom_command(
  OUTPUT ${DOMPDF_ZIP_PATH}
  COMMAND
    ${CMAKE_COMMAND}
      -D URL=https://github.com/dompdf/dompdf/releases/download/v3.1.0/dompdf-3.1.0.zip
      -D HASH=cafb29ad27308660e1650ff54c9c4ffc67a289afb89a59d754d6649c58bd7ad1
      -D OUTPUT=${DOMPDF_ZIP_PATH}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/get_dompdf.cmake"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/get_dompdf.cmake"
  VERBATIM
)
add_custom_command(
  OUTPUT ${PHP_EXE_PATH}
  COMMAND
    ${CMAKE_COMMAND}
      -D URL_LINUX=https://dl.static-php.dev/static-php-cli/bulk/php-8.4.11-cli-linux-x86_64.tar.gz
      -D URL_WINDOWS=https://dl.static-php.dev/static-php-cli/windows/spc-max/php-8.4.11-cli-win.zip
      -D HASH_LINUX=556bd531fd4f7b9ea3c2b6743e94eeb35c458d61464aee662fcf15e0fde6ea94
      -D HASH_WINDOWS=02090fe570c5fa2adcfdf06310ba2dbadba72cf56a0bd4de9e7a1c26623bdc62
      -D EXE_HASH_LINUX=e60f8ce58554a2f3f59f6e8df664a75ad8d889f23dc9611d803b17cd0ac51c81
      -D EXE_HASH_WINDOWS=e471df4804f5705d1a66653379d379d8ff649012710cc11b3a2f9fd223760d80
      -D EXTERN_DIR=${CMAKE_CURRENT_SOURCE_DIR}/external
      -D OUTPUT=${PHP_EXE_PATH}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/get_phpcli.cmake"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/get_phpcli.cmake"
  VERBATIM
)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/embed_resources.cpp"
  COMMAND
    resource_generator -o ${CMAKE_CURRENT_BINARY_DIR} ${DOMPDF_ZIP_PATH} ${PHP_EXE_PATH}
  DEPENDS
    resource_generator
    ${DOMPDF_ZIP_PATH}
    ${PHP_EXE_PATH}
  VERBATIM
)
add_library(embedded_resources STATIC embed_resources.cpp)
target_include_directories(embedded_resources PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
target_compile_features(embedded_resources PRIVATE cxx_std_20)


# Define CLI Executable target
add_executable(dompdfcli dompdfcli_main.cpp)
target_compile_features(dompdfcli PRIVATE cxx_std_20)
target_compile_definitions(dompdfcli PRIVATE
    DOMPDF_VERSION="${DOMPDF_VERSION}"
    PHPCLI_VERSION="${PHPCLI_VERSION}"
)
target_include_directories(dompdfcli PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(dompdfcli PRIVATE
    Boost::program_options
    Boost::nowide 
    Boost::process 
    Boost::asio 
    Boost::predef 
    Boost::tokenizer
    embedded_resources
    cmake_timestamp
    -static
)


# Define GUI Executable target
# ...

