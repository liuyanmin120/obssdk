#include "AreaCaptureSourceProperty.h"
#include "BroadcastEngineImpl.h"
#include "SceneManager.h"
#include "PublicHeader.h"

std::shared_ptr<IAreaCaptureSourceProperty> IAreaCaptureSourceProperty::create(const char *src_name, bool create_source)
{
	if (src_name == nullptr)
	{
		return nullptr;
	}

	return std::make_shared<AreaCaptureSourcePropertyImpl>(src_name, create_source);
}

AreaCaptureSourcePropertyImpl::AreaCaptureSourcePropertyImpl(const char *src_name, bool create_source)
	:m_sourceName(src_name), m_props(nullptr)
{
	OBS_GET_SOURCE_BY_NAME(src_name);
	if (!source && create_source)
	{
		g_pBroadcastEngine->addSource(Broadcast::AREA_CAPTURE_SOURCE, m_sourceName.c_str(),
			g_pBroadcastEngine->scene_manager()->getCurrentSceneName());
	}
}

AreaCaptureSourcePropertyImpl::~AreaCaptureSourcePropertyImpl()
{

}

Broadcast::ESourceType AreaCaptureSourcePropertyImpl::type()
{
	return Broadcast::AREA_CAPTURE_SOURCE;
}

const char * AreaCaptureSourcePropertyImpl::id()
{
	auto strId = g_pBroadcastEngine->lookupByType(Broadcast::AREA_CAPTURE_SOURCE);
	return strId.c_str();
}

const char * AreaCaptureSourcePropertyImpl::name()
{
	return m_sourceName.c_str();
}

void AreaCaptureSourcePropertyImpl::update()
{

}

int AreaCaptureSourcePropertyImpl::setAreaCaptureParam(const Broadcast::SRect & rect)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());

	if (source == NULL)
	{
		return -1;
	}

	auto props = obs_source_properties(source);
	auto prop = obs_properties_get(props, "area_capture");

	obs_data_set_int(settings, "x", rect.x);
	obs_data_set_int(settings, "y", rect.y);
	obs_data_set_int(settings, "width", rect.width);
	obs_data_set_int(settings, "height", rect.height);
	obs_source_update(source, settings);

	obs_property_modified(prop, settings);

	obs_properties_destroy(props);
	return 0;
}

bool AreaCaptureSourcePropertyImpl::getAreaCaptureParam(Broadcast::SRect & rect)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(name());

	if (source == NULL)
	{
		return -1;
	}

	rect.x = obs_data_get_int(settings, "x");
	rect.y = obs_data_get_int(settings, "y");
	rect.width = obs_data_get_int(settings, "width");
	rect.height = obs_data_get_int(settings, "height");
	if (!rect.width || !rect.height)
	{
		rect.x = obs_data_get_default_int(settings, "x");
		rect.y = obs_data_get_default_int(settings, "y");
		rect.width = obs_data_get_default_int(settings, "width");
		rect.height = obs_data_get_default_int(settings, "height");
		return false;
	}
	else
	{
		return true;
	}
}

int AreaCaptureSourcePropertyImpl::setCursorVisible(bool isShow)
{
	return WindowCaptureSourceProperty::setCursorVisible(isShow);
}

bool AreaCaptureSourcePropertyImpl::getCursorVisibility()
{
	return WindowCaptureSourceProperty::getCursorVisibility();
}

bool AreaCaptureSourcePropertyImpl::getCompatibility()
{
	return WindowCaptureSourceProperty::getCompatibility();
}

int AreaCaptureSourcePropertyImpl::setCompatibility(bool bCompatibility)
{
	return WindowCaptureSourceProperty::setCompatibility(bCompatibility);
}
