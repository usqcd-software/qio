add_library(qio)
target_sources(qio PRIVATE  
   qio/QIO_close_read.c
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
   qio/QIO_host_utils.c
   dml/DML_crc32.c 
   dml/DML_utils.c
   lrl/LRL_main.c
)
   
if( QIO_ENABLE_PARALLEL_BUILD )
  target_sources(qio PRIVATE 
  	dml/DML_parscalar.c 
  	dml/DML_route.c
  )
else()
  target_sources(qio PRIVATE
  	dml/DML_scalar.c 
  ) 
endif()

set_target_properties(qio PROPERTIES 
		C_STANDARD 99
		C_EXTENSIONS OFF)

target_link_libraries(qio PUBLIC CLime::lime)
if(QIO_ENABLE_PARALLEL_BUILD)
  target_link_libraries(qio PUBLIC QMP::qmp)
endif()

target_include_directories(qio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<INSTALL_INTERFACE:include>)

# generated includes
target_include_directories(qio PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
  $<INSTALL_INTERFACE:include>)
 
 
set_target_properties(qio PROPERTIES C_STANDARD 99)
set_target_properties(qio PROPERTIES C_EXTENSIONS OFF)
if( QIO_SANITIZER_OPTS )
<<<<<<< HEAD
  target_compile_options(lime PUBLIC ${QIO_SANITIZER_OPTS} )
  target_link_options(lime PUBLIC ${QIO_SANITIZER_OPTS} )
=======
  target_compile_options(qio PRIVATE ${QIO_SANITIZER_OPTS} )
  target_link_options(qio PUBLIC ${QIO_SANITIZER_OPTS} )
>>>>>>> origin/extended
endif()
# Install the library and the targets 
# into the export for the targets
install(TARGETS qio EXPORT QIOTargets
  ARCHIVE DESTINATION lib
  LIBRARY  DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

#install the export description of the targets
install(EXPORT QIOTargets
		FILE QIOTargets.cmake
		NAMESPACE QIO::
		DESTINATION lib/cmake/QIO)  

add_library(QIO::qio ALIAS qio)
