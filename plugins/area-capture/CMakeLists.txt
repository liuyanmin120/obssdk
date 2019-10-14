project(area-capture)

set(area-capture_HEADERS
	obfuscate.h
	hook-helpers.h
	inject-library.h
	cursor-capture.h
	graphics-hook-info.h
	window-helpers.h
	dc-capture.h)

set(area-capture_SOURCES
	dc-capture.c
	obfuscate.c
	inject-library.c
	cursor-capture.c
	game-capture.c
	window-helpers.c
	monitor-capture.c
	window-capture.c
	load-graphics-offsets.c
	duplicator-monitor-capture.c
	plugin-main.c)

add_library(area-capture MODULE
	${area-capture_SOURCES}
	${area-capture_HEADERS})
target_link_libraries(area-capture
	libobs
	ipc-util
	psapi)

install_obs_plugin_with_data(area-capture data)