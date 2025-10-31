cmake_minimum_required(VERSION 3.31)

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
set(DOMPDF_VERSION "3.1.0" CACHE STRING "")
set(DOMPDF_RELEASE_URL "https://github.com/dompdf/dompdf/releases/download/v3.1.0/dompdf-3.1.0.zip" CACHE STRING "")
set(DOMPDF_RELEASE_HASH "cafb29ad27308660e1650ff54c9c4ffc67a289afb89a59d754d6649c58bd7ad1" CACHE STRING "")
set(PHPCLI_VERSION "8.0.30" CACHE STRING "")
set(PHPCLI_RELEASE_LINUX_URL "https://dl.static-php.dev/static-php-cli/common/php-8.0.30-cli-linux-x86_64.tar.gz" CACHE STRING "")
set(PHPCLI_RELEASE_LINUX_AR_HASH "1a5b1d5e60bca9e0c06557640f878e88bd52ac4eb28e8a118103867b084e8ff8" CACHE STRING "")
set(PHPCLI_RELEASE_LINUX_EXE_HASH "85b3876353dc47b278e68bb764fa6280deaf08b47e328f71b08c6f6f18559b14" CACHE STRING "")
set(PHPCLI_RELEASE_WINDOWS_URL "https://dl.static-php.dev/static-php-cli/windows/spc-max/php-8.0.30-cli-win.zip" CACHE STRING "")
set(PHPCLI_RELEASE_WINDOWS_AR_HASH "67c98a95790d2dd9f79b051c2b6cab0cf9bb4d4cc0b4c5401c991bc99c99ac82" CACHE STRING "")
set(PHPCLI_RELEASE_WINDOWS_EXE_HASH "03d692f3ce76a36641d7395c44ad40af62f3a2ce4d16fb4a61314a4120a8914f" CACHE STRING "")
enable_testing()


# Check Architecture
set(PROJECT_SUPPORTED_ARCHITECTURES "x86_64;AMD64;IA64")
if( NOT CMAKE_HOST_SYSTEM_PROCESSOR IN_LIST PROJECT_SUPPORTED_ARCHITECTURES )
  message(FATAL_ERROR "Architecture '${CMAKE_HOST_SYSTEM_PROCESSOR}' not supported by this project")
endif()


# Check compiler ID
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "Only GNU Compiler Collection supported by this project")
endif()


# Check Crosscompiling
if(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "Crosscompiling not supported by this project")
endif()


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
find_package(Boost 1.86.0 REQUIRED COMPONENTS program_options nowide tokenizer predef)


# Get timestamp library
CPMAddPackage("gh:abramov7613/cmake_timestamp@1.0.3")
if (NOT cmake_timestamp_ADDED)
  message(FATAL_ERROR "Can't get cmake_timestamp library")
endif()


# Define target for embedded resources
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
      -D URL=${DOMPDF_RELEASE_URL}
      -D HASH=${DOMPDF_RELEASE_HASH}
      -D OUTPUT=${DOMPDF_ZIP_PATH}
      -D CMAKE_TLS_VERIFY=${CMAKE_TLS_VERIFY}
      -P "${CMAKE_CURRENT_SOURCE_DIR}/get_dompdf.cmake"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/get_dompdf.cmake"
  VERBATIM
)
add_custom_command(
  OUTPUT ${PHP_EXE_PATH}
  COMMAND
    ${CMAKE_COMMAND}
      -D URL_LINUX=${PHPCLI_RELEASE_LINUX_URL}
      -D URL_WINDOWS=${PHPCLI_RELEASE_WINDOWS_URL}
      -D HASH_LINUX=${PHPCLI_RELEASE_LINUX_AR_HASH}
      -D HASH_WINDOWS=${PHPCLI_RELEASE_WINDOWS_AR_HASH}
      -D EXE_HASH_LINUX=${PHPCLI_RELEASE_LINUX_EXE_HASH}
      -D EXE_HASH_WINDOWS=${PHPCLI_RELEASE_WINDOWS_EXE_HASH}
      -D EXTERN_DIR=${CMAKE_CURRENT_SOURCE_DIR}/external
      -D OUTPUT=${PHP_EXE_PATH}
      -D CMAKE_TLS_VERIFY=${CMAKE_TLS_VERIFY}
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


# Define target for project executable
add_executable(${PROJECT_NAME} dompdfcli_main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DOMPDF_VERSION="${DOMPDF_VERSION}"
    PHPCLI_VERSION="${PHPCLI_VERSION}"
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::program_options
    Boost::nowide
    Boost::predef
    Boost::tokenizer
    embedded_resources
    cmake_timestamp
    -static
)


# Define tests
add_test(NAME test1
    COMMAND
      ${PROJECT_NAME}
      --no-clean
      --isRemoteEnabled=1
      "${CMAKE_CURRENT_SOURCE_DIR}/test/test1.html"
      .
)
add_test(NAME test2
    COMMAND
      ${PROJECT_NAME}
      --no-clean
      "${CMAKE_CURRENT_SOURCE_DIR}/test/test2.html"
      .
)
