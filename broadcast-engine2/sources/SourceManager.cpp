#include "Sourcemanager.h"
#include "platform/platform.hpp"
#include "display-helpers.h"
#include "obs-audio-controls.h"
#include "util/platform.h"
#include <vector>
#include "BroadcastEngineImpl.h"
#include "PublicHeader.h"

#ifndef WIN32
#include "obsconfig.h"
#endif
#ifdef __APPLE__
#include "platform/platform.hpp"
#endif
//#include "obs-scene.h"

#include <future>
#define LOG_OFFSET_DB  6.0f
#define LOG_RANGE_DB   96.0f
/* equals -log10f(LOG_OFFSET_DB) */
#define LOG_OFFSET_VAL -0.77815125038364363f
/* equals -log10f(-LOG_RANGE_DB + LOG_OFFSET_DB) */
#define LOG_RANGE_VAL  -2.00860017176191756f

int GetAppdataPath(char *path, size_t size, const char *name)
{
	return os_get_config_path(path, size, name);
}

SourceManager::SourceManager()
	:dPixelRatio(1)
	, m_offset(0)
	, reconnecting(false)
{
	resetReconnectOffset();

	int ans = initConfig();

	if (ans != 0) {
		blog(LOG_ERROR, "INIT,Failed to load basic.ini");
	}
}

SourceManager::~SourceManager()
{
	blog(LOG_INFO,"close SourceManager");
	char path[512];
#ifdef WIN32
	os_get_config_path(path, sizeof(path), "Broadcast\\basic.ini");
#else
    os_get_config_path(path, sizeof(path), "Broadcast/basic.ini");
#endif
	
	for (std::map<const char *, SAudioCallbackData*>::iterator it = volmeterMap.begin();
		it != volmeterMap.end();
		++it)
	{
		SAudioCallbackData *data = (*it).second;

		if (data)
		{
			obs_volmeter_remove_callback(data->volmeter, OBSVolumeLevel, this);
			obs_volmeter_destroy(data->volmeter);
			delete data;
		}
	}

	volmeterMap.clear();

	config_save_safe(basicConfig, "tmp", nullptr);

	signal_handler_disconnect(obs_get_signal_handler(), "source_remove",
		SourceManager::SourceRemoved, this);
	signal_handler_disconnect(obs_get_signal_handler(), "channel_change",
		SourceManager::ChannelChanged, this);
	signal_handler_disconnect(obs_get_signal_handler(), "source_activate",
		SourceManager::SourceActivated, this);
	signal_handler_disconnect(obs_get_signal_handler(), "source_deactivate",
		SourceManager::SourceDeactivated, this);
	signal_handler_disconnect(obs_get_signal_handler(), "source_rename",
		SourceManager::SourceRenamed, this);

	//outputHandler.reset();
	//obs_scene_release(scene); 
	//m_service = nullptr;

	//    delete outputHandler;
	//    outputHandler.reset();
	//    blog(LOG_ERROR,"%s---------------------%d",__FUNCTION__,__LINE__);

	obs_enter_graphics();
	gs_vertexbuffer_destroy(box);
	gs_vertexbuffer_destroy(boxLeft);
	gs_vertexbuffer_destroy(boxTop);
	gs_vertexbuffer_destroy(boxRight);
	gs_vertexbuffer_destroy(boxBottom);
	gs_vertexbuffer_destroy(circle);
	obs_leave_graphics();

	obs_set_output_source(0, nullptr);
	obs_set_output_source(1, nullptr);
	obs_set_output_source(2, nullptr);
	obs_set_output_source(3, nullptr);
	obs_set_output_source(4, nullptr);
	obs_set_output_source(5, nullptr);

	auto cb = [](void *unused, obs_source_t *source)
	{
		obs_source_remove(source);
		UNUSED_PARAMETER(unused);
		return true;
	};

	obs_enum_sources(cb, nullptr);
}

void SourceManager::createPreview(std::shared_ptr<SourceRenderInfo> info)
{
	if (!info)
	{
		blog(LOG_INFO, "render info is empty!");
		return;
	}
	if (m_render_info_list.count(info->source_name) != 0)
	{
		blog(LOG_INFO, "source render is exist!");
		return;
	}
	gs_init_data init_data = {};

	init_data.cx = uint32_t(info->rect.cx);
	init_data.cy = uint32_t(info->rect.cy);
	init_data.format = GS_RGBA;
	init_data.zsformat = GS_ZS_NONE;
	init_data.window = info->window;

	info->display = obs_display_create(&init_data, GREY_COLOR_BACKGROUND);

	if (info->display)
	{
		obs_display_add_draw_callback(info->display,
			SourceManager::renderingPreview, info.get());
	}

	m_render_info_list[info->source_name] = info;
}

#define MAIN_SEPARATOR \
    "====================================================================="

int SourceManager::init()
{
	char path[512];
	GetAppdataPath(path, sizeof(path), "Broadcast");

	if (!obs_startup(config_get_string(g_pBroadcastEngine->Config(), "General",
		"Language"), path, nullptr)) {
		blog(LOG_ERROR, "Failed to initialize core module");
		return Broadcast::CORE_MODULE_INITIAL_ERROR;
	}

	if (!ResetAudio())
	{
		blog(LOG_ERROR, "INIT,Failed to initialize audio");
		return Broadcast::AUDIO_INITIAL_ERROR;
	}

	int ret = ResetVideo();

	switch (ret) {
	case OBS_VIDEO_MODULE_NOT_FOUND: {
		blog(LOG_ERROR, "INIT,Failed to initialize video:  Graphics module not found");
		return Broadcast::VIDEO_MODULE_NOT_FOUND_ERROR;
	}
	case OBS_VIDEO_NOT_SUPPORTED: {
		blog(LOG_ERROR, "INIT,"
			"Failed to initialize video:  Required graphics API "
			"functionality not found on these drivers or "
			"unavailable on this equipment");
		return Broadcast::VIDEO_NOT_SUPPORTED_ERROR;
	}
	case OBS_VIDEO_INVALID_PARAM: {
		blog(LOG_ERROR, "INIT,Failed to initialize video:  Invalid parameters");
		return Broadcast::VIDEO_INVALID_PARAM_ERROR;
	}
	default:
		if (ret != OBS_VIDEO_SUCCESS) {
			blog(LOG_ERROR, "INIT,Failed to initialize video:  Unspecified error");
			return Broadcast::UNSPECIFIED_ERROR;
		}
	}

	InitOBSCallbacks();
	//    blog(LOG_ERROR,"%s,%d",__FUNCTION__,__LINE__);
	//    InitHotkeys();

	obs_load_all_modules();
	blog(LOG_INFO, MAIN_SEPARATOR);

	//    ResetOutputs();
	//    CreateHotkeys();

	if (!InitService()) {
		blog(LOG_ERROR, "INIT,Failed to initialize service");
		return Broadcast::SEVERICE_INTINAL_ERROR;
	}

	InitPrimitives();
	//CreateDefaultScene();
	//createDisplay();
	return Broadcast::INITIAL_OK;
}

static inline bool HasAudioDevices(const char *source_id)
{
	//    const char *output_id = source_id;
	obs_properties_t *props = obs_get_source_properties(source_id);
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t *devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

static const double scaled_vals[] =
{
	1.0,
	1.25,
	(1.0 / 0.75),
	1.5,
	(1.0 / 0.6),
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0,
	0.0
};


int SourceManager::initConfig()
{
	char path[512];
	if (GetAppdataPath(path, sizeof(path), "Broadcast") <= 0) {
		blog(LOG_WARNING, "initConfig GetAppdataPath Failed %s", path);
		return -9;
	}
	if (!os_file_exists(path)) {
		if (os_mkdir(path) == MKDIR_ERROR) {
			blog(LOG_WARNING, "initConfig Failed to create directory %s", path);
			return -9;
		}
	}
#ifdef WIN32
	if (GetAppdataPath(path, sizeof(path), "Broadcast\\basic.ini") <= 0)
#else
	if (GetAppdataPath(path, sizeof(path), "Broadcast/basic.ini") <= 0)
#endif
		return -9;

	if (basicConfig.Open(path, CONFIG_OPEN_ALWAYS) != CONFIG_SUCCESS) {
		return -1;
	}

	//    bool hasDesktopAudio = HasAudioDevices(OUTPUT_AUDIO_SOURCE);
	//    bool hasInputAudio   = HasAudioDevices(INPUT_AUDIO_SOURCE);

	std::vector<MonitorInfo> monitors;
	GetMonitors(monitors);


	if (!monitors.size()) {
		//        OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
		//                          "technically shouldn't be possible.");
		return -2;
	}

	uint32_t cx = monitors[0].cx;
	uint32_t cy = monitors[0].cy;
	screenWidth = cx;
	screenHeight = cy;


	// add encoder
	config_set_default_string(basicConfig, "SimpleOutput", "StreamEncoder",
		SIMPLE_ENCODER_X264);
	config_set_default_string(basicConfig, "SimpleOutput", "RecEncoder",
		SIMPLE_ENCODER_X264);


	/* ----------------------------------------------------- */

	config_set_default_string(basicConfig, "Output", "Mode", "Simple");

	config_set_default_string(basicConfig, "SimpleOutput", "FilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "SimpleOutput", "RecFormat",
		"flv");
	config_set_default_uint(basicConfig, "SimpleOutput", "VBitrate",
		500);
	config_set_default_uint(basicConfig, "SimpleOutput", "ABitrate", 160);
	config_set_default_bool(basicConfig, "SimpleOutput", "UseAdvanced",
		false);
	config_set_default_bool(basicConfig, "SimpleOutput", "EnforceBitrate",
		true);
	config_set_default_string(basicConfig, "SimpleOutput", "Preset",
		"veryfast");
	config_set_default_string(basicConfig, "SimpleOutput", "NVENCPreset",
		"hq");
	config_set_default_string(basicConfig, "SimpleOutput", "RecQuality",
		"Stream");
	config_set_default_bool(basicConfig, "SimpleOutput", "RecRB", false);
	config_set_default_int(basicConfig, "SimpleOutput", "RecRBTime", 20);
	config_set_default_int(basicConfig, "SimpleOutput", "RecRBSize", 512);
	config_set_default_string(basicConfig, "SimpleOutput", "RecRBPrefix",
		"Replay");

	config_set_default_bool(basicConfig, "AdvOut", "ApplyServiceSettings",
		true);
	config_set_default_bool(basicConfig, "AdvOut", "UseRescale", false);
	config_set_default_uint(basicConfig, "AdvOut", "TrackIndex", 1);
	config_set_default_string(basicConfig, "AdvOut", "Encoder", "obs_x264");

	config_set_default_string(basicConfig, "AdvOut", "RecType", "Standard");

	config_set_default_string(basicConfig, "AdvOut", "RecFilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "AdvOut", "RecFormat", "flv");
	config_set_default_bool(basicConfig, "AdvOut", "RecUseRescale",
		false);
	config_set_default_uint(basicConfig, "AdvOut", "RecTracks", (1 << 0));
	config_set_default_string(basicConfig, "AdvOut", "RecEncoder",
		"none");

	config_set_default_bool(basicConfig, "AdvOut", "FFOutputToFile",
		true);
	config_set_default_string(basicConfig, "AdvOut", "FFFilePath",
		GetDefaultVideoSavePath().c_str());
	config_set_default_string(basicConfig, "AdvOut", "FFExtension", "mp4");
	config_set_default_uint(basicConfig, "AdvOut", "FFVBitrate", 500);
	config_set_default_uint(basicConfig, "AdvOut", "FFVGOPSize", 250);
	config_set_default_bool(basicConfig, "AdvOut", "FFUseRescale",
		false);
	config_set_default_bool(basicConfig, "AdvOut", "FFIgnoreCompat",
		false);
	config_set_default_uint(basicConfig, "AdvOut", "FFABitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "FFAudioMixes", 1);

	config_set_default_uint(basicConfig, "AdvOut", "Track1Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track2Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track3Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track4Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track5Bitrate", 160);
	config_set_default_uint(basicConfig, "AdvOut", "Track6Bitrate", 160);

	config_set_default_bool(basicConfig, "AdvOut", "RecRB", false);
	config_set_default_uint(basicConfig, "AdvOut", "RecRBTime", 20);
	config_set_default_int(basicConfig, "AdvOut", "RecRBSize", 512);

	config_set_default_uint(basicConfig, "Video", "BaseCX", cx);
	config_set_default_uint(basicConfig, "Video", "BaseCY", cy);

	/* don't allow BaseCX/BaseCY to be susceptible to defaults changing */
	if (!config_has_user_value(basicConfig, "Video", "BaseCX") ||
		!config_has_user_value(basicConfig, "Video", "BaseCY")) {
		config_set_uint(basicConfig, "Video", "BaseCX", cx);
		config_set_uint(basicConfig, "Video", "BaseCY", cy);
		config_save_safe(basicConfig, "tmp", nullptr);
	}

	config_set_default_string(basicConfig, "Output", "FilenameFormatting",
		"%CCYY-%MM-%DD %hh-%mm-%ss");

	config_set_default_bool(basicConfig, "Output", "DelayEnable", false);
	config_set_default_uint(basicConfig, "Output", "DelaySec", 20);
	config_set_default_bool(basicConfig, "Output", "DelayPreserve", true);

	config_set_default_bool(basicConfig, "Output", "Reconnect", true);
	config_set_default_uint(basicConfig, "Output", "RetryDelay", 10);
	config_set_default_uint(basicConfig, "Output", "MaxRetries", 20);

	config_set_default_string(basicConfig, "Output", "BindIP", "default");
	config_set_default_bool(basicConfig, "Output", "NewSocketLoopEnable",
		false);
	config_set_default_bool(basicConfig, "Output", "LowLatencyEnable",
		false);

	int i = 0;
	uint32_t scale_cx = cx;
	uint32_t scale_cy = cy;

	/* use a default scaled resolution that has a pixel count no higher
	* than 1280x720 */
	while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0) {
		double scale = scaled_vals[i++];
		scale_cx = uint32_t(double(cx) / scale);
		scale_cy = uint32_t(double(cy) / scale);
	}

	config_set_default_uint(basicConfig, "Video", "OutputCX", scale_cx);
	config_set_default_uint(basicConfig, "Video", "OutputCY", scale_cy);

	/* don't allow OutputCX/OutputCY to be susceptible to defaults
	* changing */
	if (!config_has_user_value(basicConfig, "Video", "OutputCX") ||
		!config_has_user_value(basicConfig, "Video", "OutputCY")) {
		config_set_uint(basicConfig, "Video", "OutputCX", scale_cx);
		config_set_uint(basicConfig, "Video", "OutputCY", scale_cy);
		config_save_safe(basicConfig, "tmp", nullptr);
	}

	config_set_default_uint(basicConfig, "Video", "FPSType", 1);
	config_set_default_string(basicConfig, "Video", "FPSCommon", "15");
	config_set_default_uint(basicConfig, "Video", "FPSInt", 15);
	config_set_default_uint(basicConfig, "Video", "FPSNum", 15);
	config_set_default_uint(basicConfig, "Video", "FPSDen", 1);
	config_set_default_string(basicConfig, "Video", "ScaleType", "bicubic");
	config_set_default_string(basicConfig, "Video", "ColorFormat", "NV12");
	config_set_default_string(basicConfig, "Video", "ColorSpace", "601");
	config_set_default_string(basicConfig, "Video", "ColorRange",
		"Partial");

	config_set_default_string(basicConfig, "Audio", "MonitoringDeviceId",
		"default");
	config_set_default_string(basicConfig, "Audio", "MonitoringDeviceName",
		Str("Basic.Settings.Advanced.Audio.MonitoringDevice"
			".Default"));
	config_set_default_uint(basicConfig, "Audio", "SampleRate", 48000); // 44100 rtc 默认为48000,必须一致
	config_set_default_string(basicConfig, "Audio", "ChannelSetup",
		"Stereo");
	config_set_default_double(basicConfig, "Audio", "MeterDecayRate",
		VOLUME_METER_DECAY_FAST);
	config_set_default_uint(basicConfig, "Audio", "PeakMeterType", 0);
	return 0;
}

void SourceManager::InitHotkeys()
{
	struct obs_hotkeys_translations t = {};
	t.insert = Str("Hotkeys.Insert");
	t.del = Str("Hotkeys.Delete");
	t.home = Str("Hotkeys.Home");
	t.end = Str("Hotkeys.End");
	t.page_up = Str("Hotkeys.PageUp");
	t.page_down = Str("Hotkeys.PageDown");
	t.num_lock = Str("Hotkeys.NumLock");
	t.scroll_lock = Str("Hotkeys.ScrollLock");
	t.caps_lock = Str("Hotkeys.CapsLock");
	t.backspace = Str("Hotkeys.Backspace");
	t.tab = Str("Hotkeys.Tab");
	t.print = Str("Hotkeys.Print");
	t.pause = Str("Hotkeys.Pause");
	t.left = Str("Hotkeys.Left");
	t.right = Str("Hotkeys.Right");
	t.up = Str("Hotkeys.Up");
	t.down = Str("Hotkeys.Down");
#ifdef _WIN32
	t.meta = Str("Hotkeys.Windows");
#else
	t.meta = Str("Hotkeys.Super");
#endif
	t.menu = Str("Hotkeys.Menu");
	t.space = Str("Hotkeys.Space");
	t.numpad_num = Str("Hotkeys.NumpadNum");
	t.numpad_multiply = Str("Hotkeys.NumpadMultiply");
	t.numpad_divide = Str("Hotkeys.NumpadDivide");
	t.numpad_plus = Str("Hotkeys.NumpadAdd");
	t.numpad_minus = Str("Hotkeys.NumpadSubtract");
	t.numpad_decimal = Str("Hotkeys.NumpadDecimal");
	t.apple_keypad_num = Str("Hotkeys.AppleKeypadNum");
	t.apple_keypad_multiply = Str("Hotkeys.AppleKeypadMultiply");
	t.apple_keypad_divide = Str("Hotkeys.AppleKeypadDivide");
	t.apple_keypad_plus = Str("Hotkeys.AppleKeypadAdd");
	t.apple_keypad_minus = Str("Hotkeys.AppleKeypadSubtract");
	t.apple_keypad_decimal = Str("Hotkeys.AppleKeypadDecimal");
	t.apple_keypad_equal = Str("Hotkeys.AppleKeypadEqual");
	t.mouse_num = Str("Hotkeys.MouseButton");
	obs_hotkeys_set_translations(&t);

	obs_hotkeys_set_audio_hotkeys_translations(Str("Mute"), Str("Unmute"),
		Str("Push-to-mute"), Str("Push-to-talk"));

	obs_hotkeys_set_sceneitem_hotkeys_translations(
		Str("SceneItemShow"), Str("SceneItemHide"));

	obs_hotkey_enable_callback_rerouting(true);
	//	obs_hotkey_set_callback_routing_func(OBSBasic::HotkeyTriggered, this);
}

bool SourceManager::ResetAudio()
{
	struct obs_audio_info ai;
	ai.samples_per_sec = (uint32_t)config_get_uint(basicConfig, "Audio",
		"SampleRate");

	const char *channelSetupStr = config_get_string(basicConfig,
		"Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else
		ai.speakers = SPEAKERS_STEREO;

	//    ai.buffer_ms = config_get_uint(basicConfig, "Audio", "BufferingTime");

	return obs_reset_audio(&ai);
}

void SourceManager::InitOBSCallbacks()
{
	signal_handler_connect(obs_get_signal_handler(), "source_remove",
		SourceManager::SourceRemoved, this);
	signal_handler_connect(obs_get_signal_handler(), "channel_change",
		SourceManager::ChannelChanged, this);
	signal_handler_connect(obs_get_signal_handler(), "source_activate",
		SourceManager::SourceActivated, this);
	signal_handler_connect(obs_get_signal_handler(), "source_deactivate",
		SourceManager::SourceDeactivated, this);
	signal_handler_connect(obs_get_signal_handler(), "source_rename",
		SourceManager::SourceRenamed, this);
}

void SourceManager::ResetOutputs()
{
	//ProfileScope("OBSBasic::ResetOutputs");
	FUNC_ENTER;
	const char *mode = config_get_string(basicConfig, "Output", "Mode");
	bool advOut = astrcmpi(mode, "Advanced") == 0;

	if (!outputHandler || !outputHandler->Active()) {
		outputHandler.reset();
		outputHandler.reset(advOut ?
			CreateAdvancedOutputHandler(this) :
			CreateSimpleOutputHandler(this));

		/*delete replayBufferButton;

		if (outputHandler->replayBuffer) {
			replayBufferButton = new QPushButton(
				QTStr("Basic.Main.StartReplayBuffer"),
				this);
			replayBufferButton->setCheckable(true);
			connect(replayBufferButton.data(),
				&QPushButton::clicked,
				this,
				&OBSBasic::ReplayBufferClicked);

			replayBufferButton->setProperty("themeID", "replayBufferButton");
			ui->buttonsVLayout->insertWidget(2, replayBufferButton);
		}

		if (sysTrayReplayBuffer)
			sysTrayReplayBuffer->setEnabled(
				!!outputHandler->replayBuffer);*/
	}
	else {
		outputHandler->Update();
	}
	FUNC_LEAVE;
}

bool SourceManager::StreamingActive()
{
	if (outputHandler && outputHandler->StreamingActive()) 
	{
		return true;
	}
	return false;
}

void SourceManager::StreamingStart() 
{
	
}

int SourceManager::stopStream(bool bForce) 
{
	FUNC_ENTER;
	if (outputHandler && outputHandler->StreamingActive()) 
	{
		outputHandler->StopStreaming(bForce);
		return 0;
	}
	return -1;

	FUNC_LEAVE;
}

void SourceManager::startStream()
{
	FUNC_ENTER;
	if (outputHandler) 
	{
		outputHandler->StartStreaming(m_service);
	}
	FUNC_LEAVE;
}

void SourceManager::startRecord()
{
	FUNC_ENTER;
	if (outputHandler)
	{
		outputHandler->StartRecording();
	}
	FUNC_LEAVE;
}

void SourceManager::stopRecord()
{
	FUNC_ENTER;
	if (outputHandler)
	{
		outputHandler->StopRecording();
	}
	FUNC_LEAVE;
}

bool SourceManager::InitService()
{
	m_service = obs_service_create("rtmp_custom", "default_service", nullptr, nullptr);

	if (!m_service)
		return false;

	return true;
}

bool SourceManager::updateService(std::string server, std::string key)
{
	if (server.c_str() == nullptr || key.c_str() == nullptr)
		return false;

	obs_data_t *settings = obs_service_get_settings(m_service);
	obs_data_set_string(settings, "server", server.c_str());
	obs_data_set_string(settings, "key", key.c_str());

	blog(LOG_ERROR, "%s: %s/%s", __FUNCTION__, server.c_str(), key.c_str());

	obs_service_update(m_service, settings);
	obs_data_release(settings);
	return true;
}

void SourceManager::InitPrimitives()
{
	obs_enter_graphics();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(0.0f, 0.0f);
	box = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	boxLeft = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(1.0f, 0.0f);
	boxTop = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxRight = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f);
	boxBottom = gs_render_save();

	gs_render_start(true);
	for (int i = 0; i <= 360; i += (360 / 20)) {
		float pos = RAD(float(i));
		gs_vertex2f(cosf(pos), sinf(pos));
	}
	circle = gs_render_save();

	obs_leave_graphics();
}

void SourceManager::ResetAudioDevice(const char *sourceId, const char *deviceName,
	const char *deviceDesc, int channel)
{
	//    blog(LOG_ERROR,"%s,%s:%d",__FUNCTION__,deviceName,__LINE__);
	const char *deviceId = config_get_string(basicConfig, "Audio",
		deviceName);
	obs_source_t *source;
	obs_data_t *settings;
	bool same = false;

	source = obs_get_output_source(channel);
	if (source) {
		//        blog(LOG_ERROR,"%s,%s:%d",__FUNCTION__,deviceName,__LINE__);

		settings = obs_source_get_settings(source);
		const char *curId = obs_data_get_string(settings, "device_id");

		same = (strcmp(curId, deviceId) == 0);

		obs_data_release(settings);
		obs_source_release(source);
	}

	if (!same)
		obs_set_output_source(channel, nullptr);

	if (!same && strcmp(deviceId, "disabled") != 0) {
		//        blog(LOG_ERROR,"%s,%s:%d",__FUNCTION__,deviceName,__LINE__);

		obs_data_t *settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		source = obs_source_create(sourceId, deviceDesc, settings, nullptr);
		obs_data_release(settings);

		obs_set_output_source(channel, source);
		obs_source_release(source);
	}
}

static void GetConfigFPS(ConfigFile &basicConfig, uint32_t &num, uint32_t &den)
{
	uint32_t type = config_get_uint(basicConfig, "Video", "FPSType");

	if (type == 1) //"Integer"
	{
		num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSInt");
		den = 1;
	}
	else if (type == 2) //"Fraction"
	{
		num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNum");
		den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSDen");
	}
	else if (false) //"Nanoseconds", currently not implemented
	{
		num = 1000000000;
		den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNS");
	}
	else {
		const char *val = config_get_string(basicConfig, "Video", "FPSCommon");

		if (strcmp(val, "10") == 0) {
			num = 10;
			den = 1;
		}
		else if (strcmp(val, "20") == 0) {
			num = 20;
			den = 1;
		}
		else if (strcmp(val, "25") == 0) {
			num = 25;
			den = 1;
		}
		else if (strcmp(val, "29.97") == 0) {
			num = 30000;
			den = 1001;
		}
		else if (strcmp(val, "48") == 0) {
			num = 48;
			den = 1;
		}
		else if (strcmp(val, "59.94") == 0) {
			num = 60000;
			den = 1001;
		}
		else if (strcmp(val, "60") == 0) {
			num = 60;
			den = 1;
		}
		else {
			num = 30;
			den = 1;
		}
	}
}
static inline enum video_format GetVideoFormatFromName(const char *name)
{
	if (astrcmpi(name, "I420") == 0)
		return VIDEO_FORMAT_I420;
	else if (astrcmpi(name, "NV12") == 0)
		return VIDEO_FORMAT_NV12;
	//#if 0 //currently unsupported
	else if (astrcmpi(name, "YVYU") == 0)
		return VIDEO_FORMAT_YVYU;
	else if (astrcmpi(name, "YUY2") == 0)
		return VIDEO_FORMAT_YUY2;
	else if (astrcmpi(name, "UYVY") == 0)
		return VIDEO_FORMAT_UYVY;
	//#endif
	else
		return VIDEO_FORMAT_BGRA;
}
static inline enum obs_scale_type GetScaleType(ConfigFile &basicConfig)
{
	const char *scaleTypeStr = config_get_string(basicConfig,
		"Video", "ScaleType");

	if (astrcmpi(scaleTypeStr, "bilinear") == 0)
		return OBS_SCALE_BILINEAR;
	else if (astrcmpi(scaleTypeStr, "lanczos") == 0)
		return OBS_SCALE_LANCZOS;
	else
		return OBS_SCALE_BICUBIC;
}
inline int AttemptToResetVideo(struct obs_video_info *ovi, void *data)
{
	blog(LOG_ERROR, "%s:%d,%d", __FUNCTION__, ovi->base_width, ovi->base_height);

	int ret = obs_reset_video(ovi /*[](struct obs_source_frame *frame, void *param)
	{
		//SourceManager *manager = (SourceManager*)(param);
		//CC::VideoFrame cc_frame;
		//cc_frame.data_size = frame->data_size;
		//cc_frame.timestamp = frame->timestamp;
		//cc_frame.format = frame->format;
		//cc_frame.width = frame->width;
		//cc_frame.height = frame->height;
		//cc_frame.data_size = frame->data_size;
		//for (int i = 0; i < 3; ++i)
		//{
		//	cc_frame.plane[i] = frame->data[i];
		//	cc_frame.stride[i] = frame->linesize[i];
		//}
		//if (g_pCCEngine)
		//{// add by sun. triger obs.vidio.io:frame->webrtc.
		//	g_pCCEngine->m_pCallback->OnVideoMixerCallback(&cc_frame);

		//}
	}, data*/);

	return ret;
}

int SourceManager::ResetVideo()
{
	if (reconnecting.load())
	{
		return -8;
	}

	struct obs_video_info ovi;
	int ret;

	GetConfigFPS(basicConfig, ovi.fps_num, ovi.fps_den);

	const char *colorFormat = config_get_string(basicConfig, "Video",
		"ColorFormat");
	const char *colorSpace = config_get_string(basicConfig, "Video",
		"ColorSpace");
	const char *colorRange = config_get_string(basicConfig, "Video",
		"ColorRange");

	const char *renderer = config_get_string(g_pBroadcastEngine->Config(), "Video",
		"Renderer");
	ovi.graphics_module = astrcmpi(renderer, "Direct3D 11") == 0 ? DL_D3D11 : DL_OPENGL;

	ovi.base_width = (uint32_t)config_get_uint(basicConfig,
		"Video", "BaseCX");
	ovi.base_height = (uint32_t)config_get_uint(basicConfig,
		"Video", "BaseCY");
	ovi.output_width = (uint32_t)config_get_uint(basicConfig,
		"Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(basicConfig,
		"Video", "OutputCY");
	ovi.output_format = GetVideoFormatFromName(colorFormat);
	ovi.colorspace = astrcmpi(colorSpace, "601") == 0 ?
		VIDEO_CS_601 : VIDEO_CS_709;
	ovi.range = astrcmpi(colorRange, "Full") == 0 ?
		VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = GetScaleType(basicConfig);

	if (ovi.base_width == 0 || ovi.base_height == 0) {
		ovi.base_width = BASEWIDTH;
		ovi.base_height = BASEHEIGHT;
		config_set_uint(basicConfig, "Video", "BaseCX", BASEWIDTH);
		config_set_uint(basicConfig, "Video", "BaseCY", BASEHEIGHT);
	}

	if (ovi.output_width == 0 || ovi.output_height == 0) {
		ovi.output_width = ovi.base_width;
		ovi.output_height = ovi.base_height;
		config_set_uint(basicConfig, "Video", "OutputCX",
			ovi.base_width);
		config_set_uint(basicConfig, "Video", "OutputCY",
			ovi.base_height);
	}
#ifdef _WIN32
#define IS_WIN32 1
#else
#define IS_WIN32 0
#endif

#ifdef WIN32
	ret = AttemptToResetVideo(&ovi, this);
	if (IS_WIN32 && ret != OBS_VIDEO_SUCCESS) {
		/* Try OpenGL if DirectX fails on windows */
		if (astrcmpi(ovi.graphics_module, DL_OPENGL) != 0) {
			blog(LOG_INFO, "Failed to initialize obs video (%d) "
				"with graphics_module='%s', retrying "
				"with graphics_module='%s'",
				ret, ovi.graphics_module,
				DL_OPENGL);
			ovi.graphics_module = DL_OPENGL;
			ret = AttemptToResetVideo(&ovi, this);
		}
	}
#else
    std::promise<int> promise;
    std::thread([&]()
                {
                    std::this_thread::yield();
                    ret = AttemptToResetVideo(&ovi, this);
                    if (IS_WIN32 && ret != OBS_VIDEO_SUCCESS) {
                        /* Try OpenGL if DirectX fails on windows */
                        if (astrcmpi(ovi.graphics_module, DL_OPENGL) != 0) {
                            blog(LOG_INFO, "Failed to initialize obs video (%d) "
                                 "with graphics_module='%s', retrying "
                                 "with graphics_module='%s'",
                                 ret, ovi.graphics_module,
                                 DL_OPENGL);
                            ovi.graphics_module = DL_OPENGL;
                            ret = AttemptToResetVideo(&ovi, this);
                        }
                    }
                    promise.set_value(ret);
                }).detach();
    ret = promise.get_future().get();
#endif
	return ret;
}

void SourceManager::renderingPreview(void *data, uint32_t cx, uint32_t cy)
{
#if 1
	SourceRenderInfo *info = reinterpret_cast<SourceRenderInfo*>(data);

	obs_source_t *source = obs_get_source_by_name(info->source_name.c_str());
	if (!source)
	{
		return;
	}

	//    obs_video_info ovi;
	//    obs_get_video_info(&ovi);
	//    window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));

	uint32_t sourceCX = obs_source_get_width(source);
	uint32_t sourceCY = obs_source_get_height(source);
	sourceCX = std::max(sourceCX, 1u);
	sourceCY = std::max(sourceCY, 1u);

	int   x, y;
	int   newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY),
		-100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(source);

	gs_projection_pop();
	gs_viewport_pop();

	obs_source_release(source);
#endif
}

void SourceManager::DrawBackdrop(float cx, float cy)
{
	if (!box)
		return;

	gs_effect_t    *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_eparam_t    *color = gs_effect_get_param_by_name(solid, "color");
	gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

	vec4 colorVal;
	vec4_set(&colorVal, 0.59f, 0.59f, 0.59f, 1.0f);
	gs_effect_set_vec4(color, &colorVal);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);
	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_scale3f(float(cx), float(cy), 1.0f);

	gs_load_vertexbuffer(box);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
	gs_technique_end_pass(tech);
	gs_technique_end(tech);

	gs_load_vertexbuffer(nullptr);
}

void SourceManager::ActivateAudioSource(const char * srcName)
{


	auto source = obs_get_source_by_name(srcName);
	if (!source)
		return;
	const char* id = obs_source_get_id(source);
	if (NULL == id)
	{
		obs_source_release(source);
		return;
	}

	g_pBroadcastEngine->engineObserver()->OnActivateAudioSource(srcName);
#ifdef  WIN32
	//wasapi_input_capture 麦克风    wasapi_output_capture 扬声器
	if (strcmp(AUDIOOUT, id) == 0
		|| strcmp("extern_audio_input", id) == 0
		|| strcmp("wasapi_input_capture", id) == 0)
#else
	if (strcmp("coreaudio_output_capture", id) == 0 || strcmp("coreaudio_input_capture", id) == 0)
#endif //WIN32
	{
		SAudioCallbackData *data = new SAudioCallbackData;
		data->manager = this;
		data->source = source;
		data->volmeter = obs_volmeter_create(OBS_FADER_LOG);
		obs_volmeter_attach_source(data->volmeter, source);
		obs_volmeter_add_callback(data->volmeter, OBSVolumeLevel, data);
		volmeterMap.emplace(srcName, data);
	}

	obs_source_release(source);

}

void SourceManager::DeactivatAudioSource(const char * srcName)
{
}

void SourceManager::addOffset(int64_t microseconds)
{
	m_offset += microseconds;
	blog(LOG_INFO, "fix duration for live by free packet:%lld", microseconds);
}

//void SourceManager::OBSVolumeLevel(void *data, float level, float mag,
//	float peak, float muted)
//{
//	//通过回调抛出去
//	
//	SAudioCallbackData* invoker = static_cast<SAudioCallbackData*>(data);
//	if (invoker)
//	{
//		const char *srcname = obs_source_get_name(invoker->source);
//		invoker->manager->notifyVolume(srcname, level, mag, peak);
//	}
//}
#include "media-io/audio-math.h"

static float iec_db_to_def(const float db)
{
	if (db == 0.0f)
		return 1.0f;
	else if (db == -INFINITY)
		return 0.0f;

	float def;

	if (db >= -9.0f)
		def = (db + 9.0f) / 9.0f * 0.25f + 0.75f;
	else if (db >= -20.0f)
		def = (db + 20.0f) / 11.0f * 0.25f + 0.5f;
	else if (db >= -30.0f)
		def = (db + 30.0f) / 10.0f * 0.2f + 0.3f;
	else if (db >= -40.0f)
		def = (db + 40.0f) / 10.0f * 0.15f + 0.15f;
	else if (db >= -50.0f)
		def = (db + 50.0f) / 10.0f * 0.075f + 0.075f;
	else if (db >= -60.0f)
		def = (db + 60.0f) / 10.0f * 0.05f + 0.025f;
	else if (db >= -114.0f)
		def = (db + 150.0f) / 90.0f * 0.025f;
	else
		def = 0.0f;

	return def;
}


static float cubic_db_to_def(const float db)
{
	if (db == 0.0f)
		return 1.0f;
	else if (db == -INFINITY)
		return 0.0f;

	return cbrtf(db_to_mul(db));
}




static float log_def_to_db(const float def)
{
	if (def >= 1.0f)
		return 0.0f;
	else if (def <= 0.0f)
		return -INFINITY;

	return -(LOG_RANGE_DB + LOG_OFFSET_DB) * powf(
		(LOG_RANGE_DB + LOG_OFFSET_DB) / LOG_OFFSET_DB, -def)
		+ LOG_OFFSET_DB;
}

static float log_db_to_def(const float db)
{
	if (db >= 0.0f)
		return 1.0f;
	else if (db <= -96.0f)
		return 0.0f;

	return (-log10f(-db + LOG_OFFSET_DB) - LOG_RANGE_VAL)
		/ (LOG_OFFSET_VAL - LOG_RANGE_VAL);
}

void SourceManager::OBSVolumeLevel(void *data,
	const float magnitude[MAX_AUDIO_CHANNELS],
	const float peak[MAX_AUDIO_CHANNELS],
	const float inputPeak[MAX_AUDIO_CHANNELS])
{
	SAudioCallbackData* invoker = static_cast<SAudioCallbackData*>(data);
	if (invoker)
	{

		auto db_2_mul = [](const float db, float &fMul, int &nCount)
		{
			if (db != -INFINITY)
			{
				fMul += log_db_to_def(db);
				nCount++;
			}
		};

		const char *srcname = obs_source_get_name(invoker->source);

		float fMagnitude = 0.0f;
		int   nMagnitude = 0;

		float fPeak = 0.0f;
		int   nPeak = 0;

		float fInputpeak = 0.0f;
		int nInputpeak = 0;
		for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) 
		{
			db_2_mul(magnitude[channelNr], fMagnitude,nMagnitude);
			db_2_mul(peak[channelNr], fPeak, nPeak);
			db_2_mul(inputPeak[channelNr], fInputpeak, nInputpeak);
		}
		auto avergeMul = [] (float & fMul,int nCount)
		{
			fMul = nCount > 0 ? fMul / nCount : 0.0f;
		};

		avergeMul(fMagnitude,nMagnitude);
		avergeMul(fPeak, nPeak);
		avergeMul(fInputpeak, nInputpeak);

		invoker->manager->notifyVolume(srcname, fMagnitude, fPeak, fInputpeak);
	}
}


config_t *SourceManager::globalConfig() const
{
	return g_pBroadcastEngine->Config();
}

obs_service_t *SourceManager::GetService()
{
	if (!m_service) {
		m_service = obs_service_create("rtmp_custom", nullptr, nullptr, nullptr);
	}
	return m_service;
}

void SourceManager::SourceRemoved(void *data, calldata_t *params)
{
	SourceManager *window = static_cast<SourceManager*>(data);
	obs_source_t *source = (obs_source_t*)calldata_ptr(params, "source");
	
	const char *id = obs_source_get_id(source);
	const char *name = obs_source_get_name(source);
	window->removeVolmeter(id, name);

	blog(LOG_INFO, "%s------%s----------------", __FUNCTION__, name);

	std::string str_id = id;
	std::string str_name = name;
	std::async([=]()
	{
		//g_pBroadcastEngine->engineObserver()->OnSourceRemoved(str_name.c_str()
		//	, window->m_pEngine->getSourceType(str_id.c_str()));

	});
}

void SourceManager::ChannelChanged(void *data, calldata_t *params)
{
}

void SourceManager::SourceActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t*)calldata_ptr(params, "source");
	uint32_t     flags = obs_source_get_output_flags(source);

	const char *sourceName = obs_source_get_name(source);

	SourceManager* invoker = reinterpret_cast<SourceManager*>(data);
	if (flags & OBS_SOURCE_AUDIO)
	{
		if (NULL != invoker)
			invoker->ActivateAudioSource(sourceName);
	}
}

void SourceManager::SourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t*)calldata_ptr(params, "source");
	uint32_t     flags = obs_source_get_output_flags(source);

	const char *sourceName = obs_source_get_name(source);

	SourceManager* invoker = static_cast<SourceManager*>(data);
	if (flags & OBS_SOURCE_AUDIO)
	{
		if (NULL != invoker)
			invoker->DeactivatAudioSource(sourceName);
	}
}

void SourceManager::SourceRenamed(void *data, calldata_t *params)
{

}

uint64_t SourceManager::durationOfLive()
{
	if (!m_start_time_point_ns)
	{
		return 0;
	}
	uint64_t now_ns = os_gettime_ns();
	uint64_t duration = 0;
	if (m_reconnect_time_point_ns)
	{
		duration = now_ns - m_reconnect_time_point_ns;
	}
	if (m_pause_recored_start_timepoint)
	{
		duration += now_ns - m_pause_recored_start_timepoint;
	}
	int64_t real_duration = (now_ns - m_start_time_point_ns) - m_offset - duration - m_record_offset + 700000000; //手动加上700ms延迟校准
	return real_duration < 0 ? 0 : real_duration;
}

void SourceManager::offsetStart(int64_t microseconds)
{
	/*if (m_pause_recored_start_timepoint.load() != 0)
	{
		return;
	}
	uint64_t encoder_cache_time = 0;
	m_offset += microseconds * 1000;
	if (outputHandler)
	{
		encoder_cache_time = ((uint64_t)outputHandler->GetEncoderCacheTimeUs()) * 1000;
		m_offset += encoder_cache_time;
	}
	m_reconnect_time_point_ns = os_gettime_ns();
	blog(LOG_INFO, "reconnect offset start notify, rtmp cache:%lld, encoder cache:%llu", microseconds,encoder_cache_time);*/
}

void SourceManager::offsetStop()
{
	if (m_pause_recored_start_timepoint.load() != 0)
	{
		return;
	}
	blog(LOG_INFO, "reconnect offset stop notify");

	if (m_reconnect_time_point_ns.load() != 0)
	{
		auto now_ns = os_gettime_ns();
		m_offset += now_ns - m_reconnect_time_point_ns;
		m_reconnect_time_point_ns = 0;
	}
}

void SourceManager::firstVideoFrameEncodePrepare(uint64_t timestamp)
{
	if (!m_start_time_point_ns)
	{
		m_start_time_point_ns = os_gettime_ns();
		m_first_video_frame_ns = timestamp;
	}

	auto now_ns = os_gettime_ns();
	//m_offset += now_ns - timestamp;
	blog(LOG_INFO, "receive first video frame, offset:%llu", m_offset.load() / 1000000);
}

void SourceManager::resetReconnectOffset()
{
	blog(LOG_INFO, "resetReconnectOffset");
	m_offset = 0;
	m_start_time_point_ns = 0;
	m_first_video_frame_ns = 0;
	m_reconnect_time_point_ns = 0;
	m_pause_recored_start_timepoint = 0;
	m_record_offset = 0;
}

void SourceManager::resetOffsetForNextRecording()
{
	blog(LOG_INFO, "resetOffsetForNextRecording");
	m_offset = 0;
	m_start_time_point_ns = os_gettime_ns();
	m_first_video_frame_ns = 0;
	m_reconnect_time_point_ns = 0;
	m_pause_recored_start_timepoint = 0;
	m_record_offset = 0;
}

const char* SourceManager::getLogoSourceName() {
	static bool tag = true;
	static char buf[100] = { 0, };
	if (tag) {
		//        time_t     now = time(0);
		//        struct tm  tstruct;
		//        tstruct = *localtime(&now);
		//        strftime(buf, sizeof(buf), "CCLOGO___%Y-%m-%d, %X", &tstruct);
		using namespace std::chrono;

		struct tm  tstruct;

		auto tp = system_clock::now();
		auto now = system_clock::to_time_t(tp);
		tstruct = *localtime(&now);

		size_t written = strftime(buf, sizeof(buf), "CC_LOGO_%X", &tstruct);
		if (std::ratio_less<system_clock::period, seconds::period>::value &&
			written && (sizeof(buf) - written) > 5) {
			auto tp_secs =
				time_point_cast<seconds>(tp);
			auto millis =
				duration_cast<milliseconds>(tp - tp_secs).count();

			snprintf(buf + written, sizeof(buf) - written, ".%03u",
				static_cast<unsigned>(millis));
		}
		tag = false;
	}
	return buf;
}

static inline long long color_to_int(unsigned alpha, unsigned red, unsigned green, unsigned blue)
{
	auto shift = [&](unsigned val, int shift)
	{
		return ((val & 0xff) << shift);
	};

	return  shift(red, 0) |
		shift(green, 8) |
		shift(blue, 16) |
		shift(alpha, 24);
}

void SourceManager::removePreview(const std::string &source_name)
{
	auto it = m_render_info_list.find(source_name);
	if (it != m_render_info_list.end())
	{
		auto info = it->second;
		obs_display_remove_draw_callback(info->display, SourceManager::renderingPreview, nullptr);
		obs_display_destroy(info->display);
		m_render_info_list.erase(it);
	}
}

void SourceManager::requestRedirectStream()
{
	//g_pBroadcastEngine->engineObserver()->requestRedirectStream();
}

void SourceManager::notifyReconnectStream()
{
	blog(LOG_INFO, "notifyReconnectStream");
	reconnecting = true;
	//m_pEngine->m_pCallback->OnReconnectStream();
}

void SourceManager::notifyReconnectStreamSuccess()
{
	blog(LOG_INFO, "notifyReconnectStreamSuccess");
	reconnecting = false;
	//m_pEngine->m_pCallback->OnReconnectStreamSuccess();
}

void SourceManager::notifyVolume(const char* srcName, float level, float mag, float peak)
{
	g_pBroadcastEngine->engineObserver()->OnAudioVolume(srcName, level, mag, peak);
}

void SourceManager::removeVolmeter(const char *id, const char * srcName)
{
	if (!id || !srcName 
		|| volmeterMap.count(srcName) == 0)
	{
		return;
	}

	g_pBroadcastEngine->engineObserver()->OnDeactivatAudioSource(srcName);
	//释放的地址从存储的map中获得
#ifdef  WIN32
	//wasapi_input_capture 麦克风    wasapi_output_capture 扬声器
	if (strcmp(AUDIOOUT, id) == 0
		|| strcmp("wasapi_input_capture", id) == 0
		|| strcmp("extern_audio_input", id) == 0)	//
#else
	if (strcmp("coreaudio_output_capture", id) == 0 || strcmp("coreaudio_input_capture", id) == 0)
#endif //WIN32
	{
		SAudioCallbackData *data = volmeterMap.at(srcName);
		if (data)
		{
			obs_volmeter_remove_callback(data->volmeter, OBSVolumeLevel, this);
			obs_volmeter_destroy(data->volmeter);
			delete data;
			volmeterMap.erase(srcName);
		}
	}
}

void SourceManager::setServerRecordPauseStatus(bool pause)
{
	if (pause)
	{
		m_pause_recored_start_timepoint = os_gettime_ns();
	}
	else
	{
		if (m_pause_recored_start_timepoint)
		{
			m_record_offset += os_gettime_ns() - m_pause_recored_start_timepoint;
		}
		m_pause_recored_start_timepoint = 0;
	}
}
