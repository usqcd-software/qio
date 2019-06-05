set(QIO_SRCS qio/QIO_close_read.c
   qio/QIO_close_read.c
   qio/QIO_close_write.c
   qio/QIO_info.c
   qio/QIO_info_ildg_record.c
   qio/QIO_info_private.c
   qio/QIO_info_usqcd_gauge.c
   qio/QIO_info_usqcd_ksprop.c
   qio/QIO_info_usqcd_prop.c
   qio/QIO_next_record.c
   qio/QIO_open_read.c
   qio/QIO_open_write.c
   qio/QIO_read.c
   qio/QIO_read_record_data.c
   qio/QIO_read_record_info.c
   qio/QIO_string.c
   qio/QIO_utils.c
   qio/QIO_write.c
   qio/QIO_host_file_conversion.c
   qio/QIO_host_utils.c)
   
set(DML_GENERIC dml/DML_crc32.c dml/DML_utils.c)
set(DML_PARSCALAR dml/DML_parscalar.c dml/DML_route.c)
set(DML_SCALAR dml/DML_scalar.c)

set(LRL_SRCS lrl/LRL_main.c)

LIST(APPEND QIO_SRCS ${LRL_SRCS} ${DML_GENERIC})

if( QIO_ENABLE_PARALLEL_BUILD )
 LIST(APPEND QIO_SRCS ${DML_PARSCALAR})
else()
 LIST(APPEND QIO_SRCS ${DML_SCALAR})
endif()

add_library(qio ${QIO_SRCS})
target_link_libraries(qio PUBLIC lime)
if( QIO_ENABLE_PARALLEL_BUILD )
  target_link_libraries(qio PUBLIC qmp)
endif()

target_include_directories(qio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>)

# generated includes
target_include_directories(qio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
  $<INSTALL_INTERFACE:include>)
  

