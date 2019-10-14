#include "MonitorCaptureSourcePropertyImpl.h"
#include "BroadcastEngineImpl.h"
#include "SceneManager.h"
#include "PublicHeader.h"

std::shared_ptr<IMonitorCaptureSourceProperty> IMonitorCaptureSourceProperty::create(const char *src_name, bool create_source)
{
	if (src_name == nullptr)
	{
		return nullptr;
	}

	return std::make_shared<MonitorCaptureSourcePropertyImpl>(src_name, create_source);
}

MonitorCaptureSourcePropertyImpl::MonitorCaptureSourcePropertyImpl(const char *src_name, bool create_source)
	:m_sourceName(src_name), m_props(nullptr)
{
	OBS_GET_SOURCE_BY_NAME(src_name);
	if (!source && create_source)
	{
		g_pBroadcastEngine->addSource(Broadcast::MONITOR_CAPTURE_SOURCE, m_sourceName.c_str(),
			g_pBroadcastEngine->scene_manager()->getCurrentSceneName());
	}
}

MonitorCaptureSourcePropertyImpl::~MonitorCaptureSourcePropertyImpl()
{

}

Broadcast::ESourceType MonitorCaptureSourcePropertyImpl::type()
{
	return Broadcast::MONITOR_CAPTURE_SOURCE;
}

const char * MonitorCaptureSourcePropertyImpl::id()
{
	auto strId = g_pBroadcastEngine->lookupByType(Broadcast::MONITOR_CAPTURE_SOURCE);
	return strId.c_str();
}

const char * MonitorCaptureSourcePropertyImpl::name()
{
	return m_sourceName.c_str();
}

void MonitorCaptureSourcePropertyImpl::update()
{

}

int MonitorCaptureSourcePropertyImpl::setCursorVisible(bool isShow)
{
	return WindowCaptureSourceProperty::setCursorVisible(isShow);
}

bool MonitorCaptureSourcePropertyImpl::getCursorVisibility()
{
	return WindowCaptureSourceProperty::getCursorVisibility();
}

bool MonitorCaptureSourcePropertyImpl::getCompatibility()
{
	return WindowCaptureSourceProperty::getCompatibility();
}

int MonitorCaptureSourcePropertyImpl::setCompatibility(bool bCompatibility)
{
	return WindowCaptureSourceProperty::setCompatibility(bCompatibility);
}

const char * MonitorCaptureSourcePropertyImpl::enumMonitors(size_t idx)
{
	FUNC_ENTER;

	const char *ans = nullptr, *id = MONITOR_ENUM;
	auto props = obs_get_source_properties(DISPLAYCAPTURE);
	obs_property_t *tmp = obs_properties_get(props, id);
	auto cnt = obs_property_list_item_count(tmp);
	if (idx < cnt) {
		ans = obs_property_list_item_name(tmp, idx);
	}

	blog(LOG_ERROR, "%s,%d,%s,%d", __FUNCTION__, __LINE__, ans, idx);
	obs_properties_destroy(props);
	return ans;
}

int MonitorCaptureSourcePropertyImpl::setMonitor(const char * monitor)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source == NULL)
		return -1;

	auto props = obs_source_properties(source);
	auto prop = obs_properties_get(props, "monitor");
	auto count = obs_property_list_item_count(prop);
	for (int i = 0; i < count; ++i) {
		if (strcmp(obs_property_list_item_name(prop, i), monitor) == 0) {
			obs_data_set_int(settings, "monitor", i);
			count = 0;
			break;
		}
	}

	obs_property_modified(prop, settings);
	obs_source_update(source, settings);
	obs_properties_destroy(props);
	return count;
}

const char * MonitorCaptureSourcePropertyImpl::getMonitor()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());
	if (source == NULL) return nullptr;

	const char *id = "monitor";

	const char *ans = nullptr;
	long long dev = obs_data_has_autoselect_value(settings, id) ?
		obs_data_get_autoselect_int(settings, id) : obs_data_get_int(settings, id);

	auto props = obs_source_properties(source);
	obs_property_t *tmp = obs_properties_get(props, id);
	auto cnt = obs_property_list_item_count(tmp);
	ans = obs_property_list_item_name(tmp, 0);

	if (dev) {
		for (int i = 0; i < cnt; ++i) {
			if (obs_property_list_item_int(tmp, i) == dev) {
				ans = obs_property_list_item_name(tmp, i);
				break;
			}
		}
	}
	obs_properties_destroy(props);

	return ans;
}
