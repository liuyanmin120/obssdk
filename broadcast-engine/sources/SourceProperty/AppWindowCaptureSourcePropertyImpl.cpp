#include "AppWindowCaptureSourcePropertyImpl.h"
#include "BroadcastEngineImpl.h"
#include "SceneManager.h"

#include "PublicHeader.h"


std::shared_ptr<IAppWindowCaptureSourceProperty> IAppWindowCaptureSourceProperty::create(const char *src_name, bool create_source)
{
	if (src_name == nullptr) 
	{
		return nullptr;
	}

	return std::make_shared<AppWindowCaptureSourcePropertyImpl>(src_name, create_source);
}

AppWindowCaptureSourcePropertyImpl::AppWindowCaptureSourcePropertyImpl(const char *src_name,bool create_source)
	:m_sourceName(src_name), m_props(nullptr)
{
	OBS_GET_SOURCE_BY_NAME(src_name);
	if (!source && create_source)
	{
		createAppWindowSource();
	}

	update();
}


AppWindowCaptureSourcePropertyImpl::~AppWindowCaptureSourcePropertyImpl()
{
	obs_properties_destroy(m_props);
}

Broadcast::ESourceType AppWindowCaptureSourcePropertyImpl::type()
{
	return Broadcast::WINDOW_CAPTURE_SOURCE;
}

const char * AppWindowCaptureSourcePropertyImpl::id()
{
	auto strId = g_pBroadcastEngine->lookupByType(Broadcast::WINDOW_CAPTURE_SOURCE);
	return strId.c_str();
}

const char * AppWindowCaptureSourcePropertyImpl::name()
{
	return m_sourceName.c_str();
}

void AppWindowCaptureSourcePropertyImpl::update()
{
	OBS_GET_SOURCE_BY_NAME(m_sourceName.c_str());

	if (m_props)
	{
		obs_properties_destroy(m_props);
		m_props = nullptr;
	}

	m_props = obs_source_properties(source);
	if (!m_props)
	{
		m_props = obs_get_source_properties(id());
	}
}

int AppWindowCaptureSourcePropertyImpl::setCursorVisible(bool isShow)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source) {

#ifdef  WIN32
		//const char *id = "capture_cursor"; cursor is is changed
		const char *id = "cursor";
#else
		const char *id = "show_cursor";//""
#endif
		obs_data_set_bool(settings, id, isShow);
		obs_source_update(source, settings);

		return 0;
	}
	return -1;

	//return WindowCaptureSourceProperty::setCursorVisible(isShow);
}

bool AppWindowCaptureSourcePropertyImpl::getCursorVisibility()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source == NULL)
		return false;

#ifdef  WIN32

	const char *id = "cursor";
#else
	const char *id = "show_cursor";//""
#endif

	bool ans = obs_data_get_bool(settings, id);

	return ans;

	//return WindowCaptureSourceProperty::getCursorVisibility();
}

bool AppWindowCaptureSourcePropertyImpl::getCompatibility()
{
	return WindowCaptureSourceProperty::getCompatibility();
}

int AppWindowCaptureSourcePropertyImpl::setCompatibility(bool bCompatibility)
{
	return WindowCaptureSourceProperty::setCompatibility(bCompatibility);
}

void AppWindowCaptureSourcePropertyImpl::enumAppWindows(std::function<bool(const char*name, const char*path)> callback)
{
	FUNC_ENTER;
	auto props = obs_get_source_properties("window_capture");
	auto prop = obs_properties_get(props, "window");
	auto count = obs_property_list_item_count(prop);
	for (size_t i = 0; i < count; ++i)
	{
		auto window = obs_property_list_item_name(prop, i);
		auto path = obs_property_list_item_string(prop, i);
		if (!callback(window, path))
		{
			break;
		}
	}
	obs_properties_destroy(props);
}

int AppWindowCaptureSourcePropertyImpl::setAppWindow(const char * window)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(m_sourceName.c_str());
	if (source == NULL) return -1;

	int ans = -3;
	const char *id = "window";
	auto props = obs_source_properties(source);
	auto *prop = obs_properties_get(props, id);
	auto cnt = obs_property_list_item_count(prop);
#ifndef WIN32
	for (int i = 0; i < cnt; ++i) {
		const char *tmpItem = obs_property_list_item_name(prop, i);
		if (tmpItem != NULL && strcmp(tmpItem, window) == 0) {
			long long val = obs_property_list_item_int(prop, i);
			//obs_data_set_string(setting, id, tmpItem);
			obs_data_set_int(settings, id, val);
			ans = 0;
			break;
		}
	}
#else
	for (size_t i = 0; i < cnt; ++i) {
		if (strcmp(obs_property_list_item_name(prop, i), window) == 0) {
			obs_data_set_string(settings, id, obs_property_list_item_string(prop, i));
			ans = 0;
			break;
		}
	}
#endif
	obs_property_modified(prop, settings);
	obs_source_update(source, settings);
	obs_properties_destroy(props);
	return 0;
}

int AppWindowCaptureSourcePropertyImpl::setAppWindow(void *hwnd)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(m_sourceName.c_str());
	if (source == NULL) return -1;
	const char *id = "window";
	std::string encodeId = obs_source_get_app_encodedId(source,hwnd);
	if (!encodeId.empty()) 
	{
		obs_data_set_string(settings, id, encodeId.c_str());
		obs_source_update(source, settings);
		return 0;
	}
	else 
	{
		blog(LOG_WARNING, "hwnd is invalid");
		return -2;
	}
}

const char * AppWindowCaptureSourcePropertyImpl::getAppWindow()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(m_sourceName.c_str());

	return obs_data_get_string(settings, "window");
}

int AppWindowCaptureSourcePropertyImpl::createAppWindowSource()
{
	return g_pBroadcastEngine->addSource(Broadcast::WINDOW_CAPTURE_SOURCE, m_sourceName.c_str(), g_pBroadcastEngine->scene_manager()->getCurrentSceneName());
}
