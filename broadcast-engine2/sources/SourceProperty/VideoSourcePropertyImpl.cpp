#include "VideoSourcePropertyImpl.h"
#include "IBroadcastEngine.h"
#include "BroadcastEngineImpl.h"
#include "SceneManager.h"

#include "obs.hpp"
#include <memory>
#include <tuple>
#include <list>

#ifdef WIN32
#define VIDEO_DEVICE_ID   "video_device_id"
#define RES_TYPE          "res_type"
#define RESOLUTION        "resolution"
#define FRAME_INTERVAL    "frame_interval"
#define VIDEO_FORMAT      "video_format"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION   "last_resolution"
#define BUFFERING_VAL     "buffering"
#define FLIP_IMAGE        "flip_vertically"
#define AUDIO_OUTPUT_MODE "audio_output_mode"
#define USE_CUSTOM_AUDIO  "use_custom_audio_device"
#define COLOR_SPACE       "color_space"
#define COLOR_RANGE       "color_range"
#define DEACTIVATE_WNS    "deactivate_when_not_showing"
#else
#define VIDEO_DEVICE_ID   "device"
#define RES_TYPE          "res_type"
#define RESOLUTION        "resolution"
#define FRAME_INTERVAL    "frame_interval"
#define VIDEO_FORMAT      "input_format"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION   "last_resolution"
#define BUFFERING_VAL     "buffering"
#define FLIP_IMAGE        "flip_vertically"
#define AUDIO_OUTPUT_MODE "audio_output_mode"
#define USE_CUSTOM_AUDIO  "use_custom_audio_device"
#define COLOR_SPACE       "color_space"
#define COLOR_RANGE       "color_range"
#define DEACTIVATE_WNS    "deactivate_when_not_showing"
#endif

#define AUDIO_DEVICE_ID   "audio_device_id"

#define OBS_GET_SOURCE_BY_NAME(src_name) \
	blog(LOG_INFO,"enter func : %s", __FUNCTION__);\
	std::unique_ptr<obs_source_t,decltype(obs_source_release)*> source_ref(obs_get_source_by_name(src_name), obs_source_release);\
	obs_source_t *source = source_ref.get();


#define OBS_SOURCE_GET_SETTINGS(source) \
	std::unique_ptr<obs_data_t,decltype(obs_data_release)*> settings_ref(obs_source_get_settings(source), obs_data_release);\
	obs_data_t *settings = settings_ref.get();


std::shared_ptr<IVideoSourceProperty> IVideoSourceProperty::create(const char *src_name, bool create_source)
{
	if (!src_name)
		return nullptr;

	std::shared_ptr<IVideoSourceProperty> ptr = std::make_shared<VideoSourcePropertyImpl>(src_name, create_source);

    return ptr;
}

VideoSourcePropertyImpl::VideoSourcePropertyImpl(const char *src_name, bool create_source)
	: m_source_name(src_name)
	, m_props(nullptr)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source && create_source)
	{
		createVideoSource();
	}
	else
	{
		update();
	}
}

VideoSourcePropertyImpl::~VideoSourcePropertyImpl()
{
	obs_properties_destroy(m_props);
}

Broadcast::ESourceType VideoSourcePropertyImpl::type()
{
	return Broadcast::VIDEO_CAPTURE_SOURCE;
}

const char * VideoSourcePropertyImpl::id()
{
#ifdef WIN32
	return "dshow_input";
#endif
#ifdef __APPLE__
	return "av_capture_input";
#endif
}

const char * VideoSourcePropertyImpl::name()
{
	return m_source_name.c_str();
}

void VideoSourcePropertyImpl::update()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());

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


void VideoSourcePropertyImpl::enumVideoDevice(std::function<bool(const char *name, const char *id)> callback)
{

    blog(LOG_ERROR,"enter func %s",__FUNCTION__);
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	auto prop = obs_properties_get(m_props, VIDEO_DEVICE_ID);
	auto count = obs_property_list_item_count(prop);
    blog(LOG_ERROR,"enum device count = %d",count);
	for (size_t i = 0; i < count; ++i)
	{
		auto name = obs_property_list_item_name(prop, i);
        if(strlen(name) == 0)
        {
            continue;
        }
		auto id = obs_property_list_item_string(prop, i);
        blog(LOG_ERROR,"current device id = %s",id);
        blog(LOG_ERROR,"current device name = %s",name);
		if (!callback(name,id))
			break;
	}
}

bool VideoSourcePropertyImpl::setVideoDevice(const char *device_id, bool update_now)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return false;
	OBS_SOURCE_GET_SETTINGS(source);

	obs_data_set_string(settings, VIDEO_DEVICE_ID, device_id);
	if (update_now)
	{
		auto props = m_props ? m_props : m_props = obs_get_source_properties(id());
		auto prop = obs_properties_get(props, VIDEO_DEVICE_ID);
		auto cnt = obs_property_list_item_count(prop);

		int ans = -3;
		for (int i = 0; i < cnt; ++i)
		{
			auto name = obs_property_list_item_name(prop, i);
			blog(LOG_INFO, name);
			if (strcmp(name, device_id) == 0) {
				m_current_camera_name = name;
				obs_data_set_string(settings, VIDEO_DEVICE_ID, obs_property_list_item_string(prop, i));
				ans = 0;
				break;
			}
		}

		obs_property_modified(prop, settings);
		obs_source_update(source, settings);
	}

	return true;
}

void VideoSourcePropertyImpl::currentVideoDevice(std::function<void(const char *name, const char *id)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);
	
	callback(m_current_camera_name.c_str(), obs_data_get_string(settings, VIDEO_DEVICE_ID));
}

void VideoSourcePropertyImpl::enumCustomAudioDevice(std::function<bool(const char *name, const char *id)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	auto prop = obs_properties_get(m_props, AUDIO_DEVICE_ID);
	auto count = obs_property_list_item_count(prop);
	for (int i = 0; i < count; ++i)
	{
		auto name = obs_property_list_item_name(prop, i);
		auto id = obs_property_list_item_string(prop, i);
		if (!callback(name, id))
			break;
	}
}

void VideoSourcePropertyImpl::setAudioDevice(const char *device_id)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;
	OBS_SOURCE_GET_SETTINGS(source);
	auto prop = obs_properties_get(m_props, AUDIO_DEVICE_ID);
	obs_data_set_string(settings, AUDIO_DEVICE_ID, device_id);

	obs_property_modified(prop, settings);
	obs_source_update(source, settings);
}

void VideoSourcePropertyImpl::currentAudioDevice(std::function<void(const char *name, const char *id)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	callback("", obs_data_get_string(settings, AUDIO_DEVICE_ID));
}

void VideoSourcePropertyImpl::setAudioOutputMode(AudioOutputMode mode)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;
	OBS_SOURCE_GET_SETTINGS(source);

	auto prop = obs_properties_get(m_props, AUDIO_OUTPUT_MODE);
	auto cur_val = obs_data_get_int(settings, AUDIO_OUTPUT_MODE);
	if (mode != cur_val&&prop)
	{
		
		obs_data_set_int(settings, AUDIO_OUTPUT_MODE, mode);
		obs_property_modified(prop, settings);
		obs_source_update(source, settings);
	}
}

IVideoSourceProperty::AudioOutputMode VideoSourcePropertyImpl::audioOutputMode()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return Capture;
	OBS_SOURCE_GET_SETTINGS(source);
	auto mode = obs_data_get_int(settings, AUDIO_OUTPUT_MODE);
	return AudioOutputMode(mode);
}

void VideoSourcePropertyImpl::setCustomAudioMode(bool use)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;
	OBS_SOURCE_GET_SETTINGS(source);
	obs_data_set_bool(settings, USE_CUSTOM_AUDIO, use);

	auto prop = obs_properties_get(m_props, USE_CUSTOM_AUDIO);
	obs_property_modified(prop, settings);
	obs_source_update(source, settings);
}

bool VideoSourcePropertyImpl::isCustomAudioMode()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return false;
	OBS_SOURCE_GET_SETTINGS(source);
	return obs_data_get_bool(settings, USE_CUSTOM_AUDIO);
}

void VideoSourcePropertyImpl::setActive(bool active)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;

	auto prop = obs_properties_get(m_props, "activate");
	obs_property_button_clicked(prop, source);
}

void VideoSourcePropertyImpl::configuration(long long opaque)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;

	//obs_source_set_camera_window(source, opaque);

	auto prop = obs_properties_get(m_props, "video_config");
	
	obs_property_button_clicked(prop, source);
}

void VideoSourcePropertyImpl::setCustomMode(bool use)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	obs_data_set_int(settings, RES_TYPE, use ? 1 : 0);
	obs_source_update(source, settings);
}

bool VideoSourcePropertyImpl::isCustomMode()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	return obs_data_get_int(settings, RES_TYPE) != 0 ;
}

void VideoSourcePropertyImpl::enumResolutions(std::function<bool(const char *resolution)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	auto prop = obs_properties_get(m_props, RESOLUTION);
	auto count = obs_property_list_item_count(prop);
	for (int i = 0; i < count; ++i)
	{
 		auto res = obs_property_list_item_name(prop, i);
		if (!callback(res))
			break;
	}
}

const char * VideoSourcePropertyImpl::currentResolution()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	return obs_data_get_string(settings, RESOLUTION);
}

void VideoSourcePropertyImpl::enumVideoFormat(std::function<bool(const char *format, int32_t index)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;
	OBS_SOURCE_GET_SETTINGS(source);
	auto prop = obs_properties_get(m_props, VIDEO_FORMAT);
	auto count = obs_property_list_item_count(prop);
	
	std::list<std::tuple<int32_t,std::string>> list;
	for (int i = 0; i < count ; ++i)
	{
		auto name = obs_property_list_item_name(prop, i);
		auto index = obs_property_list_item_int(prop,i);
		auto tp = std::make_tuple<int32_t, std::string>((int)index, std::string(name));
		list.push_back(tp);
	}
	if (callback)
	{
		for (auto tp : list)
		{
			if (!callback(std::get<1>(tp).c_str(), std::get<0>(tp)))
			{
				break;
			}
		}
	}
}

void VideoSourcePropertyImpl::setVideoFormat(int32_t index)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);
	obs_data_set_int(settings, VIDEO_FORMAT, index);
	obs_source_update(source, settings);
}

const char * VideoSourcePropertyImpl::getVideoFormat()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);
	return obs_data_get_string(settings, VIDEO_FORMAT);
}

void VideoSourcePropertyImpl::createVideoSource()
{
	blog(LOG_INFO, "leave func : %s", __FUNCTION__);
#ifdef WIN32
	const char *sourceid = "dshow_input";
	const char *id = "video_device_id";
#else
	const char *sourceid = VIDEOSOURCEID;
	const char *id = "device";
#endif

	auto setting = obs_data_create();
	m_props = obs_get_source_properties(sourceid);
	auto prop = obs_properties_get(m_props, id);

	auto cnt = obs_property_list_item_count(prop);
	int index = 0;
	const char *device_id;
	do
	{
		device_id = obs_property_list_item_string(prop, index);
		const char * ch = obs_property_list_item_name(prop, index);
		if (ch) 
		{
			m_current_camera_name = ch;
		}
			
		index++;
	} while (index < cnt && device_id);
	obs_data_set_string(setting, id, device_id);
	
	auto source = obs_source_create(sourceid, m_source_name.c_str(), setting, nullptr);

	if (source)
	{
		AddSourceData data;
		data.source = source;
		data.visible = true;
		obs_scene_atomic_update(g_pBroadcastEngine->scene_manager()->currentScene(), [](void *_data, obs_scene_t *scene)
		{
			AddSourceData *data = (AddSourceData *)_data;
			OBSSceneItem sceneitem = obs_scene_add(scene, data->source);
			obs_sceneitem_set_visible(sceneitem, data->visible);
		}, &data);
	}
	obs_data_release(setting);
	obs_source_release(source);
}

void VideoSourcePropertyImpl::setResolution(const char *resolution)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return;
	OBS_SOURCE_GET_SETTINGS(source);
	auto prop = obs_properties_get(m_props, RESOLUTION);
	obs_data_set_string(settings, RESOLUTION, resolution);

	obs_property_modified(prop, settings);
	obs_source_update(source, settings);
}
