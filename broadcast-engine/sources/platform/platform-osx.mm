/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
w
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <sstream>
#include <util/base.h>
#include <obs-config.h>
#include "platform.hpp"

//#include "obs-app.hpp"

#include <unistd.h>

#import <AppKit/AppKit.h>
#import<CoreServices/CoreServices.h>
using namespace std;

bool GetDataFilePath(const char *data, string &output)
{
	stringstream str;
	str << OBS_DATA_PATH "/obs-studio/" << data;
	output = str.str();
	return !access(output.c_str(), R_OK);
}

struct cs_rect  GetWindowRect(__unsafe_unretained id &view)
{
    //CGRect rect = [view rectValue];
    struct cs_rect rect1;
//    rect1.x = rect.origin.x;
//    rect1.y = rect.origin.y;
//    rect1.cx = rect.size.width;
//    rect1.cy = rect.size.height;
    NSView *viewN = (NSView *)view;
    
    auto rect = viewN.frame;
    //NSString *str = NSStringFromRect(rect);
    //NSLog(@"str = %@",str);
    //-(CGRect)convertRect:(CGRect)rect toView:(UIView *)view;
    //取得全局坐标
    //NSView *contentView=[NSApplication sharedApplication].mainWindow.contentView;
    //CGRect rect=[view convertRect: viewN.bounds toView:contentView];
    
    rect1.x = rect.origin.x;
    rect1.y = rect.origin.y;
    rect1.cx = rect.size.width;
    rect1.cy = rect.size.height;
    
    return rect1;
}

bool isWindowHidden(__unsafe_unretained id &view)
{
    NSView *viewN = (NSView *)view;
    
    return viewN.window != nil;
}

void GetMonitors(vector<MonitorInfo> &monitors)
{
	monitors.clear();
	for(NSScreen *screen : [NSScreen screens])
	{
		NSRect frame = [screen convertRectToBacking:[screen frame]];
		monitors.emplace_back(frame.origin.x, frame.origin.y,
				      frame.size.width, frame.size.height);
	}
}

bool InitApplicationBundle()
{
#ifdef OBS_OSX_BUNDLE
	static bool initialized = false;
	if (initialized)
		return true;

	try {
		NSBundle *bundle = [NSBundle mainBundle];
		if (!bundle)
			throw "Could not find main bundle";

		NSString *exe_path = [bundle executablePath];
		if (!exe_path)
			throw "Could not find executable path";

		NSString *path = [exe_path stringByDeletingLastPathComponent];

		if (chdir([path fileSystemRepresentation]))
			throw "Could not change working directory to "
			      "bundle path";

	} catch (const char* error) {
		blog(LOG_ERROR, "InitBundle: %s", error);
		return false;
	}

	return initialized = true;
#else
	return true;
#endif
}

string GetDefaultVideoSavePath()
{
	NSFileManager *fm = [NSFileManager defaultManager];
	NSURL *url = [fm URLForDirectory:NSMoviesDirectory
				inDomain:NSUserDomainMask
		       appropriateForURL:nil
				  create:true
				   error:nil];
	
	if (!url)
		return getenv("HOME");

	return url.path.fileSystemRepresentation;
}

std::string getSystemVersion()
{
    SInt32 major, minor, bugfix;
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    Gestalt(gestaltSystemVersionBugFix, &bugfix);
    
    NSString *systemVersion = [NSString stringWithFormat:@"%d.%d.%d",
                               major, minor, bugfix];
    return systemVersion.fileSystemRepresentation;
}

std::string getComputerInfo()
{
    NSDictionary *systemVersionDictionary =
    [NSDictionary dictionaryWithContentsOfFile:
     @"/System/Library/CoreServices/SystemVersion.plist"];
    
    NSString *systemVersion =
    [systemVersionDictionary objectForKey:@"ProductVersion"];
    return systemVersion.fileSystemRepresentation;
}

//vector<string> GetPreferredLocales()
//{
//	NSArray *preferred = [NSLocale preferredLanguages];
//
//	auto locales = GetLocaleNames();
//	auto lang_to_locale = [&locales](string lang) -> string {
//		string lang_match = "";
//
//		for (const auto &locale : locales) {
//			if (locale.first == lang.substr(0, locale.first.size()))
//				return locale.first;
//
//			if (!lang_match.size() &&
//				locale.first.substr(0, 2) == lang.substr(0, 2))
//				lang_match = locale.first;
//		}
//
//		return lang_match;
//	};
//
//	vector<string> result;
//	result.reserve(preferred.count);
//
//	for (NSString *lang in preferred) {
//		string locale = lang_to_locale(lang.UTF8String);
//		if (!locale.size())
//			continue;
//
//		if (find(begin(result), end(result), locale) != end(result))
//			continue;
//
//		result.emplace_back(locale);
//	}
//
//	return result;
//}
