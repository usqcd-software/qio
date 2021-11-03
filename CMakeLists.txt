cmake_minimum_required(VERSION 3.17)
<<<<<<< HEAD
project(QIO VERSION 2.5.0 LANGUAGES C)
=======
project(QIO VERSION 2.5.1 LANGUAGES C)
>>>>>>> origin/extended

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
<<<<<<< HEAD
=======
option(QIO_BUILD_TESTS "Enable building of test programs" ON)
>>>>>>> origin/extended

set(QIO_DML_BUF_BYTES "262144"  CACHE STRING "Maximum DML Buffer Size in bytes")
set(QMP_DIR "" CACHE STRING "QMP Install Directory")
set(CLime_DIR "" CACHE STRING "C-Lime library DIrectory")

if(QIO_ENABLE_PARALLEL_BUILD)
  message(STATUS "Setting Parallel arch to parscalar")
  set(PARALLEL_ARCH "parscalar")
  set(LOCAL_PARALLEL_ARCH "parscalar")
  set(ARCHDEF_SUBSTITUTION "ARCH_PARSCALAR")
<<<<<<< HEAD
  find_package(QMP REQUIRED)
=======
  find_package(QMP 2.5.3 REQUIRED)
>>>>>>> origin/extended
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
<<<<<<< HEAD
  if( ( CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 3.1)
     OR ( CMAKE_C_COMPILER_ID MATCHES "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8) )
      message(STATUS "QIO: Setting Sanitizer options")
      set(QIO_SANITIZER_OPTS "-fsanitize=address,undefined")
  else()
    message(STATUS "Don't know how to enable sanitizers for ${CMAKE_C_COMPILER}... Ignoring")
  endif()
=======
  include(cmake/CheckSanitizeOpts.cmake)
  check_sanitizer_options( "${QIO_ENABLE_SANITIZERS}" QIO_SANITIZER_OPTS )
  message(STATUS "QIO: Setting Sanitizer options: ${QIO_SANITIZER_OPTS}")
  set(LIME_ENABLE_SANITIZERS ${QIO_ENABLE_SANITIZERS})
>>>>>>> origin/extended
endif()  

if(CLime_DIR) 
  find_package(CLime CONFIG REQUIRED)
<<<<<<< HEAD
  # For the qio-config script
  #
  find_library( CLime_LIB_FILE  liblime.a liblime.so PATHS ${CLime_DIR}/../.. NO_DEFAULT_PATH )
=======
  find_library( CLime_LIB_FILE  liblime.a liblime.so liblime.dylib PATHS ${CLime_DIR}/../.. NO_DEFAULT_PATH )
>>>>>>> origin/extended
  message(STATUS "Lime Library found at ${CLime_LIB_FILE}" )
  get_filename_component(CLime_LIBDIR ${CLime_LIB_FILE} DIRECTORY )
  message(STATUS "Lime library directory is ${CLime_LIBDIR}")
  find_file( CLime_HEADER_FILE lime.h PATHS ${CLime_LIBDIR}/../include NO_DEFAULT_PATH )
  get_filename_component(CLime_INCDIR  ${CLime_HEADER_FILE} DIRECTORY )
  message(STATUS "Lime inc directory is ${CLime_INCDIR}")
else()
  set(QIO_INTERNAL_CLIME TRUE)
<<<<<<< HEAD
  set(LIME_ENABLE_SANITIZERS ${QIO_ENABLE_SANITIZERS})
=======
>>>>>>> origin/extended
  add_subdirectory(other_libs/c-lime)
endif()

add_subdirectory(lib)
<<<<<<< HEAD
add_subdirectory(examples)

configure_file( bin/qio-config.cmake.in bin/qio-config) 
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/qio-config
 PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
 DESTINATION bin)  
=======
if( QIO_BUILD_TESTS )
  add_subdirectory(examples)
endif()

configure_file(bin/qio-config.cmake.in ./bin/qio-config)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bin/qio-config DESTINATION bin/ PERMISSIONS
          OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
>>>>>>> origin/extended

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
<<<<<<< HEAD
    QIOVersion.cmake
=======
    QIOConfigVersion.cmake
>>>>>>> origin/extended
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

<<<<<<< HEAD
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QIOVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/QIOConfig.cmake DESTINATION lib/cmake/QIO)
=======
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QIOConfigVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/QIOConfig.cmake DESTINATION lib/cmake/QIO)
>>>>>>> origin/extended

