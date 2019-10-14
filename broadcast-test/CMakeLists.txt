cmake_minimum_required(VERSION 3.1)

project(broadcast-test)

find_package(Qt5Widgets ${FIND_MODE})
find_package(Qt5Network ${FIND_MODE})
find_package(Qt5Core ${FIND_MODE})
find_package(Qt5Gui ${FIND_MODE})

include_directories(${CMAKE_SOURCE_DIR})
function(addSrcDir dirName RESULT)
	file(GLOB_RECURSE TEMPLATE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${dirName}/*.h")
    #message(STATUS "temp file path:${TEMPLATE_FILES}")
    foreach(FILE_NAME ${TEMPLATE_FILES})
		set(TMPHEADERS ${FILE_NAME} ${TMPHEADERS})
	endforeach()
    #message(STATUS "Template files:${TMPHEADERS}")
    aux_source_directory(${dirName} TMPSOURCES)

    if(${dirName} MATCHES "/")
		string(FIND ${dirName} "/" StrIndex)
		math(EXPR StrIndex "${StrIndex} + 1}")
		string(SUBSTRING ${dirName} ${StrIndex} -1 GroupName)
		message(STATUS "dirName :${dirName} GroupName :${GroupName}")
		if(GroupName)
			source_group(${GroupName} FILES ${TMPHEADERS} ${TMPSOURCES})
		endif(GroupName)
	else()
		source_group(${dirName} FILES ${TMPHEADERS} ${TMPSOURCES})
	endif()

	set(${RESULT} ${TMPHEADERS} ${TMPSOURCES} PARENT_SCOPE)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/${dirName})
endfunction()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#ui
FILE(GLOB UI_FILES "ui/*.ui")
qt5_wrap_ui(TEST_UI_FILES ${UI_FILES})
qt5_add_resources(TEST_QRC_FILES test.qrc)
#qrc

#sources
addSrcDir("sources" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})
#sources/BroadcastEngine
addSrcDir("sources/BroadcastEngine" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})
#sources/area_capture
addSrcDir("sources/area_capture" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})

#sources/eos
addSrcDir("sources/eos" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})

add_executable(broadcast-test WIN32
	${ALL_SOURCES_AND_HEADERS}
	${TEST_UI_FILES}
	${TEST_QRC_FILES}
	)

target_link_libraries(broadcast-test
	broadcast-engine
	Qt5::Widgets
	Qt5::Network
	)

set( ENV{PATH} ${QTDIR} )

if(APPLE)
	set_property(
	TARGET ${PROJECT_NAME}
	APPEND
	PROPERTY INSTALL_RPATH
	"."
	"../Frameworks"
	"${Qt5Widgets_DIR}/../../")
	install_obs_core(${PROJECT_NAME})
endif()
