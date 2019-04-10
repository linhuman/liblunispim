cmake_minimum_required(VERSION 2.8)
include(FindPkgConfig)
project (lunispim)
add_compile_options(-Wno-write-strings)
find_package(Boost 1.46.0 REQUIRED COMPONENTS filesystem system)
if(Boost_FOUND)
	set(CMAKE_CXX_FLAGS "-lboost_system ${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_FLAGS "-lboost_filesystem ${CMAKE_CXX_FLAGS}")  
endif(Boost_FOUND)

set(PREFIX_DIR "${CMAKE_INSTALL_PREFIX}")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
	set(LIB_INSTALL_DIR "${PREFIX_DIR}/lib64")
else()
	set(LIB_INSTALL_DIR "${PREFIX_DIR}/lib")
endif()

set(DATA_INSTALL_DIR "${PREFIX_DIR}/share")
set(BIN_INSTALL_DIR "${PREFIX_DIR}/bin")
set(INC_INSTALL_DIR "${PREFIX_DIR}/include")
set(LIB_LUNISPIM "lunispim")
set(LUNISPIM_VERSION 0.1)
configure_file(
	${PROJECT_SOURCE_DIR}/lunispim.pc.in
	${PROJECT_BINARY_DIR}/lunispim.pc
	@ONLY)
add_subdirectory(src)
if(DEFINED DESTDIR)
	install(FILES ${PROJECT_BINARY_DIR}/lunispim.pc DESTINATION ${DESTDIR}${LIB_INSTALL_DIR}/pkgconfig)
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/lunispim DESTINATION ${DESTDIR}${DATA_INSTALL_DIR})
else()
	install(FILES ${PROJECT_BINARY_DIR}/lunispim.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/data/lunispim DESTINATION ${DATA_INSTALL_DIR})
endif()


