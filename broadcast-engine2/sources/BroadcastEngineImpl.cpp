#include "BroadcastEngineImpl.h"
#include "util/base.h"
#include "util/bmem.h"
#include "util/platform.h"

#include "SourceManager.h"
#include "SceneManager.h"
#include "PublicHeader.h"
#include "window-basic-main-outputs.hpp"

#include <future>

#ifdef WIN32
#include <windows.h>
#endif

#include <thread>
#include <chrono>
#include <fstream>

#define MAX_LOG_ITEM_LENGTH  4096

#define AUDIO_SOURCE_JSON "audio_source.json"
#define VIDEO_SOURCE_JSON "video_source.json"
#define OBS_SOURCE_JSON "obs_source.json"

BroadcastEngineImpl*g_pBroadcastEngine = nullptr;
std::fstream  g_logFile;
static void SourceLoaded(void *parm, obs_source_t *source);

char *changeTxtEncoding(char* szU8)
{
#ifdef WIN32
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);
	wszString[wcsLen] = '\0';

	int ansiLen = ::WideCharToMultiByte(CP_ACP, NULL, wszString, wcslen(wszString), NULL, 0, NULL, NULL);
	char* szAnsi = new char[ansiLen + 1];
	::WideCharToMultiByte(CP_ACP, NULL, wszString, wcslen(wszString), szAnsi, ansiLen, NULL, NULL);
	szAnsi[ansiLen] = '\0';

	return szAnsi;
#else
	return szU8;
#endif
}

std::string GenerateTimeDateFilename(const char *extension, bool noSpace = false)
{
	time_t    now = time(0);
	char      file[256] = {};
	struct tm *cur_time;


	cur_time = localtime(&now);
	snprintf(file, sizeof(file), "%d-%02d-%02d%c%02d-%02d-%02d.%s",
		cur_time->tm_year + 1900,
		cur_time->tm_mon + 1,
		cur_time->tm_mday,
		noSpace ? '_' : ' ',
		cur_time->tm_hour,
		cur_time->tm_min,
		cur_time->tm_sec,
		extension);

	return std::string(file);
}

static bool do_mkdir(const char *path)
{
	if (os_mkdir(path) == MKDIR_ERROR) {
		blog(LOG_ERROR, "Failed to create directory %s", path);
		return false;
	}

	return true;
}

static void create_log_file()
{
	if (g_pBroadcastEngine == nullptr) 
	{
		return;
	}

#ifdef _WIN32
	char ConfigPath[512];
	os_get_config_path(ConfigPath, 512, "Broadcast");

	char LogPath[512];
	os_get_config_path(LogPath, 512, "Broadcast\\Log");
#else

	char ConfigPath[512];
	os_get_config_path(ConfigPath, 512, "Broadcast");

	char LogPath[512];
	os_get_config_path(LogPath, 512, "Broadcast/Log");
#endif

	if (!do_mkdir(ConfigPath))
		return ;
	if (!do_mkdir(LogPath))
		return ;

	char path[512];
	static std::string currentLogFile = GenerateTimeDateFilename("txt");
#ifdef WIN32
	os_get_config_path(path, 512, ("Broadcast\\Log\\" + currentLogFile).c_str());
#else
	//os_get_config_path(path, 512, ("Broadcast/Log/" + currentLogFile).c_str());
#endif

#ifdef WIN32
	char *changeResult = changeTxtEncoding(path);

	g_logFile.open(changeResult,
		std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
#else
	g_logFile.open(path, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
#endif

	if (g_logFile.is_open()) {
		base_set_log_handler(&BroadcastEngineImpl::do_log, g_pBroadcastEngine);
	}
	else {
		blog(LOG_ERROR, "Failed to open log file");
	}
}

IBroadcastEngine * IBroadcastEngine::create(IBroadcastEngineObserver * observer)
{
	if (g_pBroadcastEngine == nullptr) 
	{
		g_pBroadcastEngine = new BroadcastEngineImpl(observer);
		create_log_file();
	}

	return g_pBroadcastEngine;
}

void IBroadcastEngine::release(IBroadcastEngine * object)
{
	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	delete object;
	blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());

}

BroadcastEngineImpl::BroadcastEngineImpl(IBroadcastEngineObserver *observer):m_pEngineObserver(observer)
{
	//base_set_log_handler(&BroadcastEngineImpl::do_log, this);

#ifdef WIN32	
	//m_lookupByType.emplace(BroadcastEngine::ESourceType::TEXT_SOURCE, "text_gdiplus");
	//m_lookupByType.emplace(BroadcastEngine::ESourceType::SPEECH_RECOGNITION_SOURCE, "speech_recognition");
#else
	//m_lookupByType.emplace(Broadcast::ESourceType::TEXT_SOURCE, "text_ft2_source");
#endif
	m_lookupByType.emplace(Broadcast::ESourceType::IMAGE_FILE_SOURCE, "image_source");
	//m_lookupByType.emplace(BroadcastEngine::ESourceType::MEDIA_FILE_SOURCE, "ffmpeg_source");
	//m_lookupByType.emplace(BroadcastEngine::ESourceType::MEDIA_NETWORK_SOURCE, "ffmpeg_source");
	m_lookupByType.emplace(Broadcast::ESourceType::WINDOW_CAPTURE_SOURCE, "window_capture");
	m_lookupByType.emplace(Broadcast::ESourceType::AREA_CAPTURE_SOURCE, "area_capture");
	//m_lookupByType.emplace(BroadcastEngine::ESourceType::MEDIA_TALK_SOURCE, "media_talk_input");
#ifdef __APPLE__	

	m_lookupByType.emplace(Broadcast::ESourceType::MONITOR_CAPTURE_SOURCE, DISPLAYCAPTURE);

	//    lookupbytype.emplace(BroadcastEngine::ESourceType::GAME_CAPTURE_SOURCE,"game_capture");
	m_lookupByType.emplace(Broadcast::ESourceType::VIDEO_CAPTURE_SOURCE, VIDEOSOURCEID);
	//lookupbytype.emplace(BroadcastEngine::ESourceType::AUDIO_INPUT_SOURCE, "wasapi_input_capture");
	m_lookupByType.emplace(Broadcast::ESourceType::AUDIO_INPUT_SOURCE, AUDIOINPUT);
	m_lookupByType.emplace(Broadcast::ESourceType::AUDIO_OUTPUT_SOURCE, AUDIOOUT);
#endif	
#ifdef WIN32

	m_lookupByType.emplace(Broadcast::ESourceType::MONITOR_CAPTURE_SOURCE, "monitor_capture");

	//    lookupbytype.emplace(BroadcastEngine::ESourceType::GAME_CAPTURE_SOURCE,"game_capture");
	m_lookupByType.emplace(Broadcast::ESourceType::VIDEO_CAPTURE_SOURCE, "dshow_input");
	//lookupbytype.emplace(BroadcastEngine::ESourceType::AUDIO_INPUT_SOURCE, "wasapi_input_capture");
	//lookupbytype.emplace(BroadcastEngine::ESourceType::AUDIO_INPUT_SOURCE, "extern_audio_input");
	m_lookupByType.emplace(Broadcast::ESourceType::AUDIO_INPUT_SOURCE, AUDIOINPUT);
	m_lookupByType.emplace(Broadcast::ESourceType::AUDIO_OUTPUT_SOURCE, "wasapi_output_capture");

#endif

	for (auto v = m_lookupByType.begin(); v != m_lookupByType.end(); ++v) {
		m_lookupByName.emplace(v->second, v->first);
	}

	m_sceneManager = std::make_shared<SceneManager>();
	m_sourceManager = std::make_shared<SourceManager>();

}

BroadcastEngineImpl::~BroadcastEngineImpl()
{
	
	m_sceneManager.reset();
	m_sourceManager.reset();

	obs_shutdown();
	m_pEngineObserver = nullptr;
	//base_set_log_handler(nullptr, nullptr);
}

static std::string CurrentTimeString()
{
	using namespace std::chrono;

	struct tm  tstruct;
	char       buf[80];

	auto tp = system_clock::now();
	auto now = system_clock::to_time_t(tp);
	tstruct = *localtime(&now);

	size_t written = strftime(buf, sizeof(buf), "%X", &tstruct);
	if (std::ratio_less<system_clock::period, seconds::period>::value &&
		written && (sizeof(buf) - written) > 5) {
		auto tp_secs =
			time_point_cast<seconds>(tp);
		auto millis =
			duration_cast<milliseconds>(tp - tp_secs).count();

		snprintf(buf + written, sizeof(buf) - written, ".%03u",
			static_cast<unsigned>(millis));
	}

	return buf;
}

bool log2File(char* str, int log_level)
{
	auto level2Str = [](int level)->std::string
	{
		switch (level)
		{
		case LOG_ERROR:
			return "Error";
		case LOG_WARNING:
			return "Warning";
		case LOG_DEBUG:
			return "Debug";
		default:
			break;
		}
		return "Info";
	};

	auto printLogMsg = [&](const char *timeString, char *str, int level)
	{
		g_logFile << timeString << "[" << level2Str(level) << "]" << str << std::endl;
	};

	char *nextLine = str;
	std::string timeString = CurrentTimeString();
	timeString += ": ";

	while (*nextLine) {
		char *nextLine = strchr(str, '\n');
		if (!nextLine)
			break;

		if (nextLine != str && nextLine[-1] == '\r') {
			nextLine[-1] = 0;
		}
		else {
			nextLine[0] = 0;
		}

		printLogMsg(timeString.c_str(), str, log_level);
		nextLine++;
		str = nextLine;
	}

	printLogMsg(timeString.c_str(), str, log_level);
	return false;
}

void BroadcastEngineImpl::do_log(int log_level, const char * msg, va_list args, void * param)
{
	char str[MAX_LOG_ITEM_LENGTH];
#ifndef _WIN32
	va_list args2;
	va_copy(args2, args);
#endif

	if (strlen(msg) >= MAX_LOG_ITEM_LENGTH)
		return;

	int l = strlen(msg);
	vsnprintf(str, 4095, msg, args);

#ifdef _WIN32
	OutputDebugStringA(str);
	OutputDebugStringA("\n");
#else
	//def_log_handler(log_level, msg, args2, nullptr)Ø;
#endif

	if (log_level <= 500)
	{
		log2File(str, log_level);
	}
}

void BroadcastEngineImpl::logSink(const std::string &msg, int log_level)
{
	if (m_pEngineObserver) 
	{
		m_pEngineObserver->onLogMsg(msg, log_level);
	}
}



int BroadcastEngineImpl::init()
{
	if (!initGlobalConfig()) 
	{
		return Broadcast::OPEN_GLOBAL_CONFIG_ERROR;
	}
	return m_sourceManager->init();
}

int BroadcastEngineImpl::load()
{
	//FUNC_ENTER;
	//int bRet = 0;
	//if (flag & AUDIO_SOURCE_FLAG) 
	//{
	//	if (!loadSourceFromSetting(getSettingFilePath(AUDIO_SOURCE_JSON)))
	//	{
	//		bRet |= AUDIO_SOURCE_FLAG;
	//	}
	//}

	//if (flag &VIDEO_SOURCE_FLAG) 
	//{

	//	if (!loadSourceFromSetting(getSettingFilePath(VIDEO_SOURCE_JSON)))
	//	{
	//		bRet |= VIDEO_SOURCE_FLAG;
	//	}
	//}
	auto sourceDataFile = getSettingFilePath(OBS_SOURCE_JSON);
	if (!os_file_exists(sourceDataFile.c_str()))
	{
		blog(LOG_INFO, "loading source,no such file:%s", sourceDataFile.c_str());
		return -1;
	}

	OBSData data = obs_data_create_from_json_file_safe(sourceDataFile.c_str(), "bak");
	if (!data) {
		blog(LOG_ERROR, "Failed to load '%s'", sourceDataFile.c_str());
		return -2;
	}


	m_baseDATA = data;
	OBSDataArray sources = obs_data_get_array(m_baseDATA, "sources");
	obs_load_sources(sources, &SourceLoaded, this);

	return 0;
}

int BroadcastEngineImpl::save()
{
	FUNC_ENTER;
	auto sourceDataFile = getSettingFilePath(OBS_SOURCE_JSON);

	const char * file = sourceDataFile.c_str();

	if (!os_file_exists(file))
	{
		std::string temp(file);
		auto pos = temp.find_last_of("/\\");
		blog(LOG_INFO, "no such path, we will create:%s", temp.substr(0, pos).c_str());
		os_mkdirs(temp.substr(0, pos).c_str());
	}
	OBSData saveData = obs_data_create();
	OBSDataArray sourcesArray = obs_save_sources_filtered([](void *data, obs_source_t *source)->bool 
	{
		std::string source_name = obs_source_get_name(source);
		if (source_name == STANDARD_AUDIO_INPUT_DEVICE || source_name == STANDARD_AUDIO_OUTPUT_DEVICE)
		{
			return true;
		}
		return false;
	}, NULL);

	auto scenceManager = std::dynamic_pointer_cast<SceneManager>(m_sceneManager);
	const char *current = obs_source_get_name(obs_scene_get_source(scenceManager->currentScene()));
	obs_data_set_string(saveData, "current_scene", current);
	obs_data_set_array(saveData, "sources", sourcesArray);

	if (!obs_data_save_json_safe(saveData, file, "tmp", "bak"))
	{
		blog(LOG_ERROR, "Could not save scene data to %s", file);
		return -1;
	}

	return 0;
}


int BroadcastEngineImpl::addSource(Broadcast::ESourceType typeIndex, const char* srcName, const char *scene_name)
{
	FUNC_ENTER;
	if (typeIndex == Broadcast::ESourceType::NONE_SOURCE) {
		blog(LOG_INFO, "calling %s,wrong source type", __FUNCTION__, typeIndex);
		return -1;
	}

	auto scene = scene_manager()->findScene(scene_name);
	if (!scene)
	{
		blog(LOG_INFO, "scene is not found :%s", scene_name);
		return -2;
	}

	int nRet;
	obs_data_t *setting = nullptr;
	if (typeIndex == Broadcast::AUDIO_INPUT_SOURCE)
	{
		return createAudioInputSource(srcName);
	}
	else if (typeIndex == Broadcast::AUDIO_OUTPUT_SOURCE) 
	{
		return createAudioOutputSource(srcName);
	}
	else if (typeIndex == Broadcast::VIDEO_CAPTURE_SOURCE)
	{
		//fix camera cannot activate bug...
		setting = obs_data_create();

#ifdef WIN32
		const char *sourceid = "dshow_input";
		const char *id = "video_device_id";
#else
		const char *sourceid = VIDEOSOURCEID;
		const char *id = "device";
#endif

		auto props = obs_get_source_properties(sourceid);
		auto prop = obs_properties_get(props, id);

		auto cnt = obs_property_list_item_count(prop);
		int index = 0;
		const char *device_id;
		do
		{
			device_id = obs_property_list_item_string(prop, index);
			index++;
		} while (index < cnt && device_id);
		obs_data_set_string(setting, id, device_id);

		obs_properties_destroy(props);
	}

	auto item = scene_manager()->findSceneItem(srcName);
	obs_source_t *source = obs_sceneitem_get_source(item);

	if (source) 
	{
		blog(LOG_INFO, "%s,source:[%s] already existed", __FUNCTION__, srcName);
		nRet = -2;
		// fix bug by wyb ,if source is exist, we should not release the ref count! 
		return nRet;
	}
	else 
	{
		source = obs_source_create(m_lookupByType.at(typeIndex).c_str()
			, srcName, setting, nullptr);
		if (source) {

			blog(LOG_INFO, "%s,source:[%s] created", __FUNCTION__, m_lookupByType.at(typeIndex).c_str());
			//            obs_add_source(source);
			AddSourceData data;
			data.source = source;
			data.visible = true;
			obs_scene_atomic_update(scene, [](void *_data, obs_scene_t *scene) 
			{
				AddSourceData *data = (AddSourceData *)_data;
				OBSSceneItem sceneitem = obs_scene_add(scene, data->source);
				obs_source_inc_showing(data->source);
			}, &data);

			nRet = 0;
		}
		else 
		{
			blog(LOG_INFO, "%s,failed to create source:[%s]", __FUNCTION__, srcName);
			nRet = -3;
		}
	}

	obs_data_release(setting);
	obs_source_release(source);

	return nRet;
}

int BroadcastEngineImpl::removeSource(const char * srcName)
{
	FUNC_ENTER;

	auto remove_audio_source_func = [](int channel)
	{
		auto source = obs_get_output_source(channel);
		if (source != NULL)
		{
			obs_source_remove(source);
			obs_source_release(source);
			obs_set_output_source(channel, nullptr);
			source = nullptr;
		}
	};
	auto item = scene_manager()->findSceneItem(srcName);
	auto source = obs_sceneitem_get_source(item);
	if (source)
	{
		//if (getSourceType(obs_source_get_id(source)) == CC::MEDIA_TALK_SOURCE && m_media_audio_dispatcher)
		//{
		//	m_media_audio_dispatcher->removeDispatchSource(source);
		//}

		obs_sceneitem_remove(item);
		obs_source_remove(source);
	}
	else if (strcmp(srcName, STANDARD_AUDIO_INPUT_DEVICE) == 0)
	{
		remove_audio_source_func(3);
	}
	else if (strcmp(srcName, STANDARD_AUDIO_OUTPUT_DEVICE) == 0)
	{
		remove_audio_source_func(1);
		//updateAudioMute();
	}
	else
	{
		return -1;
	}

	return 0;
}

bool BroadcastEngineImpl::hasSource(const char * srcName)
{
	FUNC_ENTER;
	OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(srcName);
	return source != nullptr;
}

bool BroadcastEngineImpl::hasSource(Broadcast::ESourceType type)
{
	bool exist = false;
	enumSource([&exist, type](Broadcast::ESourceType type2, const char *name)
	{
		if (type == type2)
		{
			exist = true;
			return false;
		}
		return true;
	});
	return exist;
}

Broadcast::ESourceType BroadcastEngineImpl::getSourceTypeByName(const char * srcName)
{
	return Broadcast::ESourceType();
}

Broadcast::ESourceType BroadcastEngineImpl::getSourceType(const char *id)
{
	if (id)
	{
		if (m_lookupByName.count(id) != 0)
		{
			return (Broadcast::ESourceType)m_lookupByName.at(std::string(id));
		}
		else if (strcmp(id, "dshow_audio_input") == 0 || strcmp(id, "extern_audio_input") == 0)
		{
			return Broadcast::AUDIO_INPUT_SOURCE;
		}
	}
	return Broadcast::NONE_SOURCE;
}

void BroadcastEngineImpl::enumSource(std::function<bool(Broadcast::ESourceType type, const char*src_name)> callback)
{
	std::list<OBSSource> list;
	obs_enum_sources([](void *data, obs_source_t *source)
	{
		auto list = (std::list<OBSSource>*)data;
		list->push_back(source);
		return true;
	}, &list);

	for (auto source : list)
	{
		auto type = getSourceType(obs_source_get_id(source));
#ifdef __APPLE__
		//if (type == CC::MEDIA_NETWORK_SOURCE)
		//{
		//	obs_data_t *settings = obs_source_get_settings(source);
		//	bool is_local_file = obs_data_get_bool(settings, "is_local_file");
		//	if (is_local_file)
		//	{
		//		type = CC::MEDIA_FILE_SOURCE;
		//	}
		//	obs_data_release(settings);
		//}
#endif
		if (!callback(type, obs_source_get_name(source)))
		{
			break;
		}
	}

}

static bool CenterAlignItem(obs_sceneitem_t *item, obs_bounds_type type)
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	obs_transform_info itemInfo;
	vec2_set(&itemInfo.pos, 0.0f, 0.0f);
	vec2_set(&itemInfo.scale, 1.0f, 1.0f);
	itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
	itemInfo.rot = 0.0f;

	vec2_set(&itemInfo.bounds,
		float(ovi.base_width), float(ovi.base_height));
	itemInfo.bounds_type = type;
	itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

	obs_sceneitem_set_info(item, &itemInfo);
	return true;
}

static void GetItemBox(obs_sceneitem_t *item, vec3 &tl, vec3 &br)
{
	FUNC_ENTER;
	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3_set(&tl, M_INFINITE, M_INFINITE, 0.0f);
	vec3_set(&br, -M_INFINITE, -M_INFINITE, 0.0f);

	auto GetMinPos = [&](float x, float y)
	{
		vec3 pos;
		vec3_set(&pos, x, y, 0.0f);
		vec3_transform(&pos, &pos, &boxTransform);
		vec3_min(&tl, &tl, &pos);
		vec3_max(&br, &br, &pos);
	};

	GetMinPos(0.0f, 0.0f);
	GetMinPos(1.0f, 0.0f);
	GetMinPos(0.0f, 1.0f);
	GetMinPos(1.0f, 1.0f);
}

static vec3 GetItemTL(obs_sceneitem_t *item)
{
	FUNC_ENTER;
	vec3 tl, br;
	GetItemBox(item, tl, br);
	return tl;
}

static void SetItemTL(obs_sceneitem_t *item, const vec3 &tl)
{
	FUNC_ENTER;

	vec3 newTL;
	vec2 pos;

	obs_sceneitem_get_pos(item, &pos);
	newTL = GetItemTL(item);
	pos.x += tl.x - newTL.x;
	pos.y += tl.y - newTL.y;
	obs_sceneitem_set_pos(item, &pos);
}

static void CenterToScreenItem(obs_sceneitem_t *item)
{
	vec3 tl, br, itemCenter, screenCenter, offset;
	obs_video_info ovi;

	obs_get_video_info(&ovi);

	vec3_set(&screenCenter, float(ovi.base_width),
		float(ovi.base_height), 0.0f);
	vec3_mulf(&screenCenter, &screenCenter, 0.5f);

	GetItemBox(item, tl, br);

	vec3_sub(&itemCenter, &br, &tl);
	vec3_mulf(&itemCenter, &itemCenter, 0.5f);
	vec3_add(&itemCenter, &itemCenter, &tl);

	vec3_sub(&offset, &screenCenter, &itemCenter);
	vec3_add(&tl, &tl, &offset);

	SetItemTL(item, tl);
}

static void RotateSelectedSources(obs_sceneitem_t *item, float rotate)
{
	FUNC_ENTER;
	if (!item)
		return;

	float rot = rotate;

	vec3 tl = GetItemTL(item);

	rot += obs_sceneitem_get_rot(item);
	if (rot >= 360.0f)       rot -= 360.0f;
	else if (rot <= -360.0f) rot += 360.0f;
	obs_sceneitem_set_rot(item, rot);

	SetItemTL(item, tl);
}

static void MultiplySelectedItemScale(obs_sceneitem_t *item, const vec2 &mul)
{
	FUNC_ENTER;
	if (!item)
		return;

	vec3 tl = GetItemTL(item);

	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	vec2_mul(&scale, &scale, &mul);
	obs_sceneitem_set_scale(item, &scale);

	SetItemTL(item, tl);
}

static void ResetItemTransform(obs_sceneitem_t *item)
{
	FUNC_ENTER;
	if (!item)
		return;

	obs_transform_info info;
	vec2_set(&info.pos, 0.0f, 0.0f);
	vec2_set(&info.scale, 1.0f, 1.0f);
	info.rot = 0.0f;
	info.alignment = OBS_ALIGN_TOP | OBS_ALIGN_LEFT;
	info.bounds_type = OBS_BOUNDS_NONE;
	info.bounds_alignment = OBS_ALIGN_CENTER;
	vec2_set(&info.bounds, 0.0f, 0.0f);
	obs_sceneitem_set_info(item, &info);
}

int BroadcastEngineImpl::manipulateSource(const char * srcName, Broadcast::ESourceActiton operation)
{
	FUNC_ENTER;
	if (!srcName)
	{
		blog(LOG_INFO, "source name is nullptr!");
		return -4;
	}

	auto item = scene_manager()->findSceneItem(srcName);
	if (!item)
	{
		blog(LOG_INFO, "not found item:%s", srcName);
		return -5;
	}


	switch (operation) {
	case Broadcast::ESourceActiton::ORDER_MOVE_BOTTOM: {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_BOTTOM);
		break;
	}
	case Broadcast::ESourceActiton::ORDER_MOVE_TOP: {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_TOP);
		break;
	}
	case Broadcast::ESourceActiton::ORDER_MOVE_UP: {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_UP);
		break;
	}
	case Broadcast::ESourceActiton::ORDER_MOVE_DOWN: {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_DOWN);
		break;
	}
	case Broadcast::ESourceActiton::FIT_TO_SCREEN:
	{
		CenterAlignItem(item, OBS_BOUNDS_SCALE_INNER);
		break;
	}
	case Broadcast::ESourceActiton::STRETCH_TO_SCREEN:
	{
		CenterAlignItem(item, OBS_BOUNDS_STRETCH);
		break;
	}
	case Broadcast::ESourceActiton::CENTER_TO_SCREEN:
	{
		CenterToScreenItem(item);
		break;
	}
	case Broadcast::ESourceActiton::ROTATE_90:
	{
		RotateSelectedSources(item, 90.0f);
		break;
	}
	case Broadcast::ESourceActiton::ROTATE_180:
	{
		RotateSelectedSources(item, 180.0f);
		break;
	}
	case Broadcast::ESourceActiton::ROTATE_270:
	{
		RotateSelectedSources(item, -90.0f);
		break;
	}
	case Broadcast::ESourceActiton::FLIP_HORIZONTAL: {
		vec2 scale;
		vec2_set(&scale, -1.0f, 1.0f);
		MultiplySelectedItemScale(item, scale);
		break;
	}
	case Broadcast::ESourceActiton::FLIP_VERTICAL: {
		vec2 scale;
		vec2_set(&scale, 1.0f, -1.0f);
		MultiplySelectedItemScale(item, scale);
		break;
	}
	case Broadcast::ESourceActiton::RESET_TRANSFORM:
	{
		ResetItemTransform(item);
		break;
	}
	default:
		break;
	}

	return 0;
}

bool BroadcastEngineImpl::startPreviewSource(const char * srcName, const void * Window)
{
	return false;
}

bool BroadcastEngineImpl::stopPreviewSource(const char * srcName)
{
	return false;
}

std::shared_ptr<ISceneManager> BroadcastEngineImpl::sceneManager()
{
	return m_sceneManager;
}

int BroadcastEngineImpl::getOutputSize(int & width, int & height)
{
	FUNC_ENTER;
	width = (int)config_get_uint(m_sourceManager->Config(), "Video", "OutputCX");
	height = (int)config_get_uint(m_sourceManager->Config(), "Video", "OutputCY");
	return 0;
}

int BroadcastEngineImpl::setOutputSize(int width, int height)
{
	FUNC_ENTER;
	blog(LOG_ERROR, "%s:%dx%d", __FUNCTION__, width, height);

	if (width < 1 || height < 1) return -1;

	auto cx = config_get_uint(m_sourceManager->Config(), "Video", "OutputCX");
	auto cy = config_get_uint(m_sourceManager->Config(), "Video", "OutputCY");

	if (width == cx && height == cy)
	{
		return -2;
	}

	config_set_uint(m_sourceManager->Config(), "Video", "OutputCX", width & 0xFFFFFFFC);
	config_set_uint(m_sourceManager->Config(), "Video", "OutputCY", height & 0xFFFFFFFE);
	config_set_uint(m_sourceManager->Config(), "Video", "BaseCX", width & 0xFFFFFFFC);
	config_set_uint(m_sourceManager->Config(), "Video", "BaseCY", height & 0xFFFFFFFE);
	
	m_sourceManager->saveBaseConfig();
	m_sourceManager->ResetVideo();

	auto scenceManager = std::dynamic_pointer_cast<SceneManager>(m_sceneManager);
	scenceManager->zoomAllSceneItem(height / 1.0 / cy);

	std::async([=]()
	{
		m_pEngineObserver->OnOutputSizeChange(width, height);
	});

	return 0;
}

int BroadcastEngineImpl::startStream(const char * url, const char * key)
{
	FUNC_ENTER;
	if (!m_sourceManager->updateService(url, key)) 
	{
		blog(LOG_INFO, "startStream failed! url = %s,key = %s", url, key);
		return -1;
	}

	m_sourceManager->ResetOutputs();
	m_sourceManager->startStream();
	return 0;
}

int BroadcastEngineImpl::stopStream(bool bForce)
{
	FUNC_ENTER;
	return m_sourceManager->stopStream(bForce);
}

int BroadcastEngineImpl::startRecord()
{
	m_sourceManager->ResetOutputs();
	m_sourceManager->startRecord();
	
	return 0;
}

int BroadcastEngineImpl::setRecordFilePath(const char * file_path)
{
	FUNC_ENTER;	
	if (file_path == nullptr) 
	{
		blog(LOG_ERROR, "record file path is null");
		return -1;
	}
	else 
	{
		blog(LOG_INFO, "record file path :%s",file_path);
		config_set_string(m_sourceManager->Config(), "SimpleOutput", "FilePath", file_path);
	}
	return 0;
}

const char* BroadcastEngineImpl::getRecordFilePath()
{
	FUNC_ENTER;
	auto filePath = config_get_string(m_sourceManager->Config(), "SimpleOutput", "FilePath");
	if (filePath == nullptr)
	{
		blog(LOG_ERROR, "record file path is null");
	}
	else
	{
		blog(LOG_INFO, "record file path :%s", filePath);
	}
	return filePath;
}

int BroadcastEngineImpl::setRecordFileFormat(const char * recFmt)
{
	FUNC_ENTER;
	if (m_sourceManager == nullptr|| recFmt == nullptr)
	{
		return -1;
	}
	
	config_set_string(m_sourceManager->Config(), "SimpleOutput", "RecFormat", recFmt);
	return 0;
}

const char * BroadcastEngineImpl::getRecordFileFormat()
{
	auto recFmt = config_get_string(m_sourceManager->Config(), "SimpleOutput", "RecFormat");
	if (recFmt == nullptr) 
	{
		blog(LOG_ERROR, "current record format is null");
	}
	else 
	{
		blog(LOG_INFO, "current record format is %s",recFmt);
	}
	return recFmt;
}

int BroadcastEngineImpl::stopRecord()
{
	FUNC_ENTER;
	m_sourceManager->stopRecord();
	return 0;
}

std::shared_ptr<SceneManager> BroadcastEngineImpl::scene_manager()
{
	return std::dynamic_pointer_cast<SceneManager>(m_sceneManager);
}

int BroadcastEngineImpl::createAudioInputSource(const char * src_name)
{
	FUNC_ENTER;
	//blog(LOG_INFO, "cur mic id:%s", device_id);

	auto source = obs_get_output_source(3);
	if (source != NULL) 
	{
		obs_source_release(source);
		blog(LOG_INFO, "source :%s, is exist",src_name);
		return -1;
	}

	const char *type = m_lookupByType.at(Broadcast::AUDIO_INPUT_SOURCE).c_str();
	int64_t offset = 0;
	auto *setting = obs_source_get_settings(source);
	auto filter = obs_source_get_filter_by_name(source, "gain");

	//if (source != NULL)
	//{
	//	offset = obs_source_get_sync_offset(source);
	//	obs_source_remove(source);
	//	obs_source_release(source);
	//	obs_set_output_source(3, nullptr);
	//	source = nullptr;
	//}

	if (nullptr == setting) {
		setting = obs_data_create();
	}

	source = obs_source_create(type, src_name, setting, nullptr);
	obs_source_filter_add(source, filter);
	obs_set_output_source(3, source);
	obs_source_set_sync_offset(source, offset);

	obs_data_release(setting);
	obs_source_release(source);
	obs_source_release(filter);
	return 0;
}

int BroadcastEngineImpl::createAudioOutputSource(const char * src_name)
{
	FUNC_ENTER;
	auto source = obs_get_output_source(1);
	if (source != NULL)
	{
		obs_source_release(source);
		blog(LOG_INFO, "source :%s, is exist", src_name);
		return -1;
	}

	const char *type = AUDIOOUT;
	auto setting = obs_data_create();
	//obs_data_set_string(setting, "device_id", device_id);
	//obs_data_set_string(setting, "device_name", device_name);
	source = obs_source_create(type, src_name, setting, nullptr);
	obs_set_output_source(1, source);

	obs_data_release(setting);
	obs_source_release(source);

	return 0;
}

bool BroadcastEngineImpl::initGlobalConfig()
{
	FUNC_ENTER;
	char path[512];
#ifdef WIN32
	os_get_config_path(path, 512, "Broadcast\\global.ini");
#else
	os_get_config_path(path, 512, "Broadcast/global.ini");
#endif

	int code = m_globalConfigFile.Open(path, CONFIG_OPEN_ALWAYS);
	if (code != CONFIG_SUCCESS) {
		return false;
	}

	config_set_default_string(m_globalConfigFile, "General", "Language", DEFAULT_LANG);
	//    config_set_default_uint(globalConfig, "General", "MaxLogs", 10);

#if _WIN32
	config_set_default_string(m_globalConfigFile, "Video", "Renderer", "Direct3D 11");
#else
	config_set_default_string(m_globalConfigFile, "Video", "Renderer", "OpenGL");
#endif

	config_set_default_bool(m_globalConfigFile, "BasicWindow", "PreviewEnabled",
		true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"PreviewProgramMode", false);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"SceneDuplicationMode", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"SwapScenesMode", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"SnappingEnabled", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"ScreenSnapping", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"SourceSnapping", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"CenterSnapping", false);
	config_set_default_double(m_globalConfigFile, "BasicWindow",
		"SnapDistance", 10.0);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"RecordWhenStreaming", false);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"KeepRecordingWhenStreamStops", false);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"ShowTransitions", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"ShowListboxToolbars", true);
	config_set_default_bool(m_globalConfigFile, "BasicWindow",
		"ShowStatusBar", true);

	return true;
}

std::string BroadcastEngineImpl::getSettingFilePath(const std::string & file_name)
{
	FUNC_ENTER;

	char path[512];
#ifdef WIN32
	os_get_config_path(path, 512, "Broadcast\\");
#else
	os_get_config_path(path, 512, "Broadcast/");
#endif
	return  path + file_name;
}

static void SourceLoaded(void *parm, obs_source_t *source)
{
	FUNC_ENTER;
	BroadcastEngineImpl *engine = static_cast<BroadcastEngineImpl*>(parm);
	const char *name = obs_source_get_name(source);

	auto potential_scene = obs_scene_from_source(source);

	auto tmps = obs_scene_get_source(potential_scene);

	if (potential_scene != NULL)
	{
		const char *current = obs_data_get_string(engine->getBaseData(), "current_scene");
		if (current&&name&&strcmp(current, name) == 0) {
			blog(LOG_INFO, "--%s--loading scene: %s", __FUNCTION__, name);

			obs_scene_addref(potential_scene);
			auto scenceManager = std::dynamic_pointer_cast<SceneManager>(engine->sceneManager());
			scenceManager->addScene(potential_scene);

		}

		obs_scene_release(potential_scene);
		// TODO
	}
	else if (strcmp(STANDARD_AUDIO_OUTPUT_DEVICE, name) == 0)
	{
		obs_set_output_source(1, source);
	}
	else if (strcmp(STANDARD_AUDIO_INPUT_DEVICE, name) == 0)
	{
		obs_set_output_source(3, source);
	}
}

bool BroadcastEngineImpl::loadSourceFromSetting(const std::string & file_path)
{

	if (!os_file_exists(file_path.c_str()))
	{
		blog(LOG_INFO, "loading source,no such file:%s", file_path.c_str());
		return false;
	}

	OBSData data = obs_data_create_from_json_file_safe(file_path.c_str(), "bak");
	if (!data) {
		blog(LOG_ERROR, "Failed to load '%s'", file_path.c_str());
		return false;
	}


	m_baseDATA = data;
	OBSDataArray sources = obs_data_get_array(m_baseDATA, "sources");
	obs_load_sources(sources, &SourceLoaded, this);

	return true;
}

bool BroadcastEngineImpl::saveSourceToSetting(size_t flag)
{
	FUNC_ENTER;
	std::string file_path;
	if (flag == 0) 
	{
		file_path = getSettingFilePath(AUDIO_SOURCE_JSON);
	}
	else 
	{
		file_path = getSettingFilePath(VIDEO_SOURCE_JSON);
	}

	const char * file = file_path.c_str();

	if (!os_file_exists(file)) 
	{
		std::string temp(file);
		auto pos = temp.find_last_of("/\\");
		blog(LOG_INFO, "no such path, we will create:%s", temp.substr(0, pos).c_str());
		os_mkdirs(temp.substr(0, pos).c_str());
	}
	OBSData saveData = obs_data_create();
	OBSDataArray sourcesArray = obs_save_sources();

	auto scenceManager = std::dynamic_pointer_cast<SceneManager>(m_sceneManager);
	const char *current = obs_source_get_name(obs_scene_get_source(scenceManager->currentScene()));
	obs_data_set_string(saveData, "current_scene", current);
	obs_data_set_array(saveData, "sources", sourcesArray);

	if (!obs_data_save_json_safe(saveData, file, "tmp", "bak")) 
	{
		blog(LOG_ERROR, "Could not save scene data to %s", file);
		return false;
	}
	
	return true;
}


