#include "BroadcastEngineImpl.h"
#include "SceneManager.h"
#include "PublicHeader.h"

int WindowCaptureSourceProperty::setCursorVisible(bool isShow)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source) {

#ifdef  WIN32

		const char *id = "capture_cursor";
#else
		const char *id = "show_cursor";//""
#endif
		obs_data_set_bool(settings, id, isShow);
		obs_source_update(source, settings);

		return 0;
	}
	return -1;
}

bool WindowCaptureSourceProperty::getCursorVisibility()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source == NULL)
		return false;

#ifdef  WIN32

	const char *id = "capture_cursor";
#else
	const char *id = "show_cursor";//""
#endif

	bool ans = obs_data_get_bool(settings, id);

	return ans;
}

bool WindowCaptureSourceProperty::getCompatibility()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source == NULL)
		return false;

#ifdef  WIN32

	const char *id = "compatibility";
#else
	const char *id = "show_cursor";//""
#endif

	bool ans = obs_data_get_bool(settings, id);

	return ans;
}

int WindowCaptureSourceProperty::setCompatibility(bool bCompatibility)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source) {

#ifdef  WIN32

		const char *id = "compatibility";
#else
		const char *id = "show_cursor";//""
#endif
		obs_data_set_bool(settings, id, bCompatibility);
		obs_source_update(source, settings);

		return 0;
	}
	return -1;
}