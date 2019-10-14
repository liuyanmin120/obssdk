#include "AudioSourcePropertyImpl.h"
#include "BroadcastEngineImpl.h"
#include "PublicHeader.h"
#include "obs.hpp"
#include <memory>


#ifdef WIN32
#define VIDEO_DEVICE_ID   "video_device_id"
#define RES_TYPE          "res_type"
#define RESOLUTION        "resolution"
#define FRAME_INTERVAL    "frame_interval"
#define VIDEO_FORMAT      "video_format"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION   "last_resolution"
#define BUFFERING_VAL     "buffering"
#define USE_CUSTOM_AUDIO  "use_custom_audio_device"
#define COLOR_SPACE       "color_space"
#define COLOR_RANGE       "color_range"
#endif
#define AUDIO_DEVICE_ID   "device_id"


std::shared_ptr<IAudioSourceProperty> IAudioSourceProperty::create(const char *src_name)
{
	if (!src_name)
		return nullptr;

	auto ptr = std::make_shared<AudioSourcePropertyImpl>(src_name);

	return ptr;
}

AudioSourcePropertyImpl::AudioSourcePropertyImpl(const char *src_name)
	: m_source_name(src_name)
	, m_props(nullptr)
{
	update();
}

AudioSourcePropertyImpl::~AudioSourcePropertyImpl()
{
	obs_properties_destroy(m_props);
}

Broadcast::ESourceType AudioSourcePropertyImpl::type()
{
	if (m_source_name == STANDARD_AUDIO_INPUT_DEVICE) 
	{
		return Broadcast::AUDIO_INPUT_SOURCE;
	}
	else 
	{
		return Broadcast::AUDIO_OUTPUT_SOURCE;
	}
	
}

const char * AudioSourcePropertyImpl::id()
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (source)
	{
		return obs_source_get_id(source);
	}
	return nullptr;
}

const char * AudioSourcePropertyImpl::name()
{
	return m_source_name.c_str();
}

void AudioSourcePropertyImpl::update()
{
	auto source_id = g_pBroadcastEngine->lookupByType(type());
	if (!source_id.empty())
	{
		obs_properties_destroy(m_props);
		m_props = obs_get_source_properties(source_id.c_str());
	}
}

void AudioSourcePropertyImpl::enumAudioDevice(std::function<bool(const char *name, const char *id)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);

	auto prop = obs_properties_get(m_props, AUDIO_DEVICE_ID);
	auto count = obs_property_list_item_count(prop);
	for (size_t i = 0; i < count; ++i)
	{
		auto name = obs_property_list_item_name(prop, i);
		auto id = obs_property_list_item_string(prop, i);
		if (!callback(name,id))
			break;
	}
}

bool AudioSourcePropertyImpl::setAudioDevice(const char *device_id, bool update_now)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (!source)
		return false;
	OBS_SOURCE_GET_SETTINGS(source);

	obs_data_set_string(settings, AUDIO_DEVICE_ID, device_id);
	if (update_now)
	{
		auto props = m_props ? m_props : m_props = obs_get_source_properties(id());
		auto prop = obs_properties_get(props, AUDIO_DEVICE_ID);
		auto cnt = obs_property_list_item_count(prop);

		int ans = -3;
		for (size_t i = 0; i < cnt; ++i)
		{
			blog(LOG_INFO, obs_property_list_item_name(prop, i));
			if (strcmp(obs_property_list_item_name(prop, i), device_id) == 0) {
				obs_data_set_string(settings, AUDIO_DEVICE_ID, obs_property_list_item_string(prop, i));
				ans = 0;
				break;
			}
		}

		obs_property_modified(prop, settings);
		obs_source_update(source, settings);
	}

	return true;
}

void AudioSourcePropertyImpl::currentAudioDevice(std::function<void(const char *name, const char *id)> callback)
{
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);
	
	if (m_props)
	{
		auto id = obs_data_get_string(settings, AUDIO_DEVICE_ID);
		auto prop = obs_properties_get(m_props, AUDIO_DEVICE_ID);
		auto cnt = obs_property_list_item_count(prop);

		int ans = -3;
		for (size_t i  = 0; i < cnt; ++i)
		{
			blog(LOG_INFO, obs_property_list_item_name(prop, i));
			if (strcmp(obs_property_list_item_string(prop, i), id) == 0) 
			{
				callback(obs_property_list_item_name(prop, i), obs_property_list_item_string(prop, i));
				return;
			}
		}
	}
	callback("", obs_data_get_string(settings, VIDEO_DEVICE_ID));
}

int AudioSourcePropertyImpl::getAudioVolume()
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(m_source_name.c_str());
	if (source == NULL) return 0.0;
	return obs_source_get_volume(source)*100;
}

int AudioSourcePropertyImpl::setAudioVolume(int vol)
{
	FUNC_ENTER;
	blog(LOG_INFO, "volume : %d, %s", vol, m_source_name.c_str());
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(m_source_name.c_str());
	if (source == NULL) return -1;

	if ((obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO) == 0)
	{
		return -2;
	}
	obs_source_set_volume(source, float(vol) / 100.0);
	return 0;
}

void AudioSourcePropertyImpl::setAudioPhaseReverse(bool reverse)
{
	blog(LOG_INFO, "setAudioPhaseReverse:%s", reverse ? "true" : "false");
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	OBS_SOURCE_GET_SETTINGS(source);
	if (settings)
	{
		obs_data_set_bool(settings, "phase_reverse", reverse);
		obs_source_update(source, settings);
	}
}

int AudioSourcePropertyImpl::setAudioMuted(bool bMuted) 
{
	FUNC_ENTER;
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (source == nullptr) 
	{
		return -1;
	}
	blog(LOG_INFO, "set source(%s) %s", name(), bMuted ? "mute" : "unmute");
	obs_source_set_muted(source, bMuted);
	return 0;
}

bool AudioSourcePropertyImpl::getAudioMuted() 
{
	FUNC_ENTER;
	OBS_GET_SOURCE_BY_NAME(m_source_name.c_str());
	if (source == nullptr)
	{
		blog(LOG_INFO, "source(%s) is nullptr", name());
		return false;
	}
	
	return obs_source_muted(source);
}