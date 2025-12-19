cmake_minimum_required(VERSION 3.17)
PROJECT(QIOExamples VERSION 1.0.0
	LANGUAGES C)

get_directory_property(SubBuild PARENT_DIRECTORY)

if(NOT SubBuild)
	find_package(QIO REQUIRED)
endif()

set(QIO_MESH_LIST 
	qio-convert-mesh.c 
  	qio-mesh-utilities.c 
  	layout_hyper_mesh.c 
  	qio-convert-mesh.h
)
 
set(QIO_COPY_LIST 
	qio-mesh-utilities.c 
	qio-convert-mesh.h
)

if( QIO_ENABLE_PARALLEL_BUILD )
  set( QIO_TEST_LIST qio-test1
                   qio-test2
                   qio-test3
                   qio-test4
                   qio-test5
                   qio-test6
                   qio-test7
                   qio-test8 )
  set( QIO_MPP_LIST qio-test.c 
  				qio-test-util.c 
  				layout_hyper.c 
  				qio-test.h )
           
  foreach(prog ${QIO_TEST_LIST})
  	add_executable(${prog} "${prog}.c" ${QIO_MPP_LIST})
    target_link_libraries(${prog} QIO::qio)
    set_target_properties(${prog} PROPERTIES C_STANDARD 99)
    set_target_properties(${prog} PROPERTIES C_EXTENSIONS OFF)
    install(TARGETS ${prog} DESTINATION examples ) 
  endforeach()

  add_executable(qio_convert_mesh_ppfs qio-convert-mesh-ppfs.c  ${QIO_MESH_LIST})
  target_link_libraries(qio_convert_mesh_ppfs QIO::qio)
  set_target_properties(qio_convert_mesh_ppfs PROPERTIES C_STANDARD 99)
  set_target_properties(qio_convert_mesh_ppfs PROPERTIES C_EXTENSIONS OFF)
  install(TARGETS qio_convert_mesh_ppfs DESTINATION examples ) 

  add_executable(qio_copy_mesh_ppfs qio-copy-mesh-ppfs.c  ${QIO_COPY_LIST})
  target_link_libraries(qio_copy_mesh_ppfs QIO::qio)
  set_target_properties(qio_copy_mesh_ppfs PROPERTIES C_STANDARD 99)
  set_target_properties(qio_copy_mesh_ppfs PROPERTIES C_EXTENSIONS OFF)
  install(TARGETS qio_convert_mesh_ppfs DESTINATION examples )
else()
  # Scalar build
  add_executable(qio_convert_mesh_singlefs qio-convert-mesh-singlefs.c  ${QIO_MESH_LIST})
  target_link_libraries(qio_convert_mesh_singlefs QIO::qio)
  set_target_properties(qio_convert_mesh_singlefs PROPERTIES C_STANDARD 99)
  set_target_properties(qio_convert_mesh_singlefs PROPERTIES C_EXTENSIONS OFF)
  install(TARGETS qio_convert_mesh_singlefs DESTINATION examples )
  	
  	add_executable(qio_convert_mesh_pfs qio-convert-mesh-pfs.c  ${QIO_MESH_LIST})
  target_link_libraries(qio_convert_mesh_pfs QIO::qio)
  set_target_properties(qio_convert_mesh_pfs PROPERTIES C_STANDARD 99)
  set_target_properties(qio_convert_mesh_pfs PROPERTIES C_EXTENSIONS OFF)
  install(TARGETS qio_convert_mesh_pfs DESTINATION examples )
  
  add_executable(qio_convert_nersc qio-convert-nersc.c)
  target_link_libraries(qio_convert_nersc QIO::qio)
  set_target_properties(qio_convert_nersc PROPERTIES C_STANDARD 99)
  set_target_properties(qio_convert_nersc PROPERTIES C_EXTENSIONS OFF)
  install(TARGETS qio_convert_nersc DESTINATION examples )				 
 endif()
