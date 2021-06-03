cmake_minimum_required(VERSION 3.17)
project(QIO VERSION 2.5.0 LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)

option(QIO_ENABLE_PARALLEL_IO    "Enable Parallel I/O" OFF)
option(QIO_ENABLE_PARALLEL_BUILD "Enable building for a parallel architecture" OFF)
option(QIO_ENABLE_QMP_ROUTE  "Enable using QMP_route" OFF)
option(QIO_ENABLE_OUTPUT_BUFFERING "Enable OutputBuffering" OFF)
option(QIO_ENABLE_FAST_ROUTE "Enable James Osborns Fast DML route" OFF)
option(QIO_ENABLE_SANITIZERS "Enable Undefined Behaviour and Address Sanitizers" OFF)

set(QIO_DML_BUF_BYTES "262144"  CACHE STRING "Maximum DML Buffer Size in bytes")
set(QMP_DIR "" CACHE STRING "QMP Install Directory")
set(CLime_DIR "" CACHE STRING "C-Lime library DIrectory")

if(QIO_ENABLE_PARALLEL_BUILD)
  message(STATUS "Setting Parallel arch to parscalar")
  set(PARALLEL_ARCH "parscalar")
  set(LOCAL_PARALLEL_ARCH "parscalar")
  set(ARCHDEF_SUBSTITUTION "ARCH_PARSCALAR")
  find_package(QMP REQUIRED)
else()
  message(STATUS "Setting Parallel arch to scalar")
  set(PARALLEL_ARCH "scalar")
  set(LOCAL_PARALLEL_ARCH "scalar")
  set(ARCHDEF_SUBSTITUTION "ARCH_SCALAR")
endif()

   
if( QIO_ENABLE_PARALLEL_IO )
  message(STATUS "Enabling Paralle I/O")
  set(QIO_USE_PARALLEL_READ "1")
  set(QIO_USE_PARALLEL_WRITE "1")
endif()

if( QIO_ENABLE_QMP_ROUTE )
  message(STATUS "Enabling QMP Route")
  set(QIO_USE_QMP_ROUTE "1")
endif()

if( QIO_ENABLE_OUTPUT_BUFFERING )
  message(STATUS "Enaling QIO Output Buffering")
  set(QIO_OUTPUT_BUFFERING "1")
endif()

if( QIO_ENABLE_FAST_ROUTE ) 
  message(STATUS "Using FastRoute")
  set(QIO_USE_FAST_ROUTE "1")
endif()

# system checks
check_symbol_exists("fseeko" "stdio.h" HAVE_FSEEKO)

check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

# Have symbols
check_type_size("uint16_t" UINT16_T)
check_type_size("uint32_t" UINT32_T)
check_type_size("uint64_t" UINT64_T)

# Size symbols 
check_type_size("unsigned char" SIZEOF_UNSIGNED_CHAR)
check_type_size("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size("unsigned short" SIZEOF_UNSIGNED_SHORT)

configure_file(include/qio_config_internal.cmake.h.in include/qio_config_internal.h)

# Deal with Sanitizer
if( QIO_ENABLE_SANITIZERS )
  if( ( CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 3.1)
     OR ( CMAKE_C_COMPILER_ID MATCHES "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8) )
      message(STATUS "QIO: Setting Sanitizer options")
      set(QIO_SANITIZER_OPTS "-fsanitize=address,undefined")
  else()
    message(STATUS "Don't know how to enable sanitizers for ${CMAKE_C_COMPILER}... Ignoring")
  endif()
endif()  

if(CLime_DIR) 
  find_package(CLime CONFIG REQUIRED)
else()
  set(QIO_INTERNAL_CLIME TRUE)
  set(LIME_ENABLE_SANITIZERS ${QIO_ENABLE_SANITIZERS})
  add_subdirectory(other_libs/c-lime)
endif()

add_subdirectory(lib)
add_subdirectory(examples)


  

# Install the headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION . )

# Install the generated headers
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/qio_config_internal.h DESTINATION include)

# Make the config file
include(CMakePackageConfigHelpers)
configure_package_config_file(QIOConfig.cmake.in QIOConfig.cmake INSTALL_DESTINATION lib/cmake/QIO)

# Make the version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    QIOVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QIOVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/QIOConfig.cmake DESTINATION lib/cmake/QIO)

