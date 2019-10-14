/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <util/c99defs.h>
#include <string>
#include <vector>

struct MonitorInfo {
	int32_t  x, y;
	uint32_t cx, cy;

	inline MonitorInfo() {}
	inline MonitorInfo(int32_t x, int32_t y, uint32_t cx, uint32_t cy)
		: x(x), y(y), cx(cx), cy(cy)
	{}
};

struct cs_rect {
    int x;
    int y;
    int cx;
    int cy;
};

#ifndef WIN32
struct cs_rect GetWindowRect(__unsafe_unretained id &view);
bool isWindowHidden(__unsafe_unretained id &view);
#else
std::string getSystemVersion();
std::string getComputerInfo();
#endif
/* Gets the path of obs-studio specific data files (such as locale) */
bool GetDataFilePath(const char *data, std::string &path);
void GetMonitors(std::vector<MonitorInfo> &monitors);

/* Updates the working directory for OSX application bundles */
bool InitApplicationBundle();

std::string GetDefaultVideoSavePath();

std::vector<std::string> GetPreferredLocales();
