#ifndef PUBLICHEADER_H
#define PUBLICHEADER_H

#define BASEWIDTH 1920
#define BASEHEIGHT 1080
#define MAX_DEVICE_NAME_LEN 512
#define DEFAULT_LANG "en-US"
#define SIMPLE_ENCODER_X264                    "x264"
#define SIMPLE_ENCODER_X264_LOWCPU             "x264_lowcpu"
#define SIMPLE_ENCODER_QSV                     "qsv"
#define SIMPLE_ENCODER_NVENC                   "nvenc"
#define SIMPLE_ENCODER_AMD                     "amd"
#define VOLUME_METER_DECAY_FAST        23.53

#define GREY_COLOR_BACKGROUND 0xFF4C4C4C
#ifndef __APPLE__
    #define DL_OPENGL "libobs-opengl.dll"
    #define DL_D3D9  ""
    #define DL_D3D11 "libobs-d3d11.dll"
#else
    #define DL_OPENGL "libobs-opengl.so"
    #define DL_D3D9  ""
    #define DL_D3D11 "libobs-opengl.so"
#endif

#define FUNC_ENTER blog(LOG_INFO,"enter func : %s", __FUNCTION__)
#define FUNC_LEAVE blog(LOG_INFO,"leave func : %s", __FUNCTION__)

#ifdef __APPLE__
#define BASE_PATH ".."
#else
#define BASE_PATH "../.."
#endif
#define CONFIG_PATH BASE_PATH "/config"

#define STANDARD_AUDIO_INPUT_DEVICE "standard_audio_input_device"
#define STANDARD_AUDIO_OUTPUT_DEVICE "standard_audio_output_device"

#define OBS_GET_SOURCE_AND_SETTINGS_BY_NAME(src_name) \
	std::unique_ptr<obs_source_t,decltype(obs_source_release)*> source_ref(obs_get_source_by_name(src_name), obs_source_release);\
	obs_source_t *source = source_ref.get();\
	std::unique_ptr<obs_data_t,decltype(obs_data_release)*> settings_ref(obs_source_get_settings(source), obs_data_release);\
	obs_data_t *settings = settings_ref.get();

#define OBS_GET_SOURCE_BY_NAME(src_name) \
	blog(LOG_INFO,"enter func : %s", __FUNCTION__);\
	std::unique_ptr<obs_source_t,decltype(obs_source_release)*> source_ref(obs_get_source_by_name(src_name), obs_source_release);\
	obs_source_t *source = source_ref.get();


#define OBS_SOURCE_GET_SETTINGS(source) \
	std::unique_ptr<obs_data_t,decltype(obs_data_release)*> settings_ref(obs_source_get_settings(source), obs_data_release);\
	obs_data_t *settings = settings_ref.get();
#endif // PUBLICHEADER_H
