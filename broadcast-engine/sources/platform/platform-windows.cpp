﻿/******************************************************************************
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

#include <algorithm>
#include <sstream>
#include "obs-config.h"
//#include "obs-app.hpp"
#include "platform.hpp"
using namespace std;

#include <util/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

static inline bool check_path(const char* data, const char *path,
		string &output)
{
	ostringstream str;
	str << path << data;
	output = str.str();

	printf("Attempted path: %s\n", output.c_str());

	return os_file_exists(output.c_str());
}

bool GetDataFilePath(const char *data, string &output)
{
	if (check_path(data, "data/obs-studio/", output))
		return true;

	return check_path(data, OBS_DATA_PATH "/obs-studio/", output);
}

static BOOL CALLBACK OBSMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
		LPRECT rect, LPARAM param)
{
	vector<MonitorInfo> &monitors = *(vector<MonitorInfo> *)param;

	monitors.emplace_back(
			rect->left,
			rect->top,
			rect->right - rect->left,
			rect->bottom - rect->top);

	UNUSED_PARAMETER(hMonitor);
	UNUSED_PARAMETER(hdcMonitor);
	return true;
}

void GetMonitors(vector<MonitorInfo> &monitors)
{
	monitors.clear();
	EnumDisplayMonitors(NULL, NULL, OBSMonitorEnumProc, (LPARAM)&monitors);
}

bool InitApplicationBundle()
{
	return true;
}

string GetDefaultVideoSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char    path_utf8[MAX_PATH]  = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
			path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	return string(path_utf8);
}

static vector<string> GetUserPreferredLocales()
{
	vector<string> result;

	ULONG num, length = 0;
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num,
				nullptr, &length))
		return result;

	vector<wchar_t> buffer(length);
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num,
				&buffer.front(), &length))
		return result;

	result.reserve(num);
	auto start = begin(buffer);
	auto end_  = end(buffer);
	decltype(start) separator;
	while ((separator = find(start, end_, 0)) != end_) {
		if (result.size() == num)
			break;

		char conv[MAX_PATH] = {};
		os_wcs_to_utf8(&*start, separator - start, conv, MAX_PATH);

		result.emplace_back(conv);

		start = separator + 1;
	}

	return result;
}
