cmake_minimum_required(VERSION 3.1)

#project
project(broadcast-engine)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/deps")
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}")

function(addSrcDir dirName RESULT)
	file(GLOB_RECURSE TEMPLATE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${dirName}/*.h*")
    message(STATUS "temp file path:${TEMPLATE_FILES}")
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
#sources
addSrcDir("sources" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})
#source property
addSrcDir("sources/SourceProperty" LIST)
list(APPEND ALL_SOURCES_AND_HEADERS ${LIST})

#platform
set(obs_PLATFORM_HEADERS sources/platform/platform.hpp)
if(WIN32)
	set(obs_PLATFORM_SOURCES
		sources/platform/platform-windows.cpp
		)
elseif(APPLE)
	set(obs_PLATFORM_SOURCES
		sources/platform/platform-osx.mm)

	find_library(COCOA Cocoa)
	mark_as_advanced(COCOA)
	include_directories(${COCOA})

	find_library(APPKIT AppKit)
	mark_as_advanced(APPKIT)
	include_directories(${APPKIT})
elseif(UNIX)
	set(obs_PLATFORM_SOURCES
		sources/platform/platform-x11.cpp)
endif()

source_group(platform FILES ${obs_PLATFORM_SOURCES} ${obs_PLATFORM_HEADERS})
list(APPEND ALL_SOURCES_AND_HEADERS ${obs_PLATFORM_HEADERS})
list(APPEND ALL_SOURCES_AND_HEADERS ${obs_PLATFORM_SOURCES})


add_library(broadcast-engine SHARED ${ALL_SOURCES_AND_HEADERS})

target_compile_definitions(broadcast-engine
	PUBLIC BROADCAST_ENGINE_EXPORT)

if(WIN32)
	set(EngineDepsLib
		Winmm
		Advapi32
		ws2_32
		Iphlpapi
		Shell32
		)
else()
	set(EngineDepsLib
	${COCOA}
	${APPKIT})
endif()

target_link_libraries(broadcast-engine
	libobs
	${EngineDepsLib})

install_obs_core(broadcast-engine)
