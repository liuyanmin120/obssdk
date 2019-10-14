#ifndef SOURCEMANAGER_H_
#define SOURCEMANAGER_H_

#include "obs.hpp"
#include <util/util.hpp>
#include "util/dstr.hpp"
#include "graphics/matrix4.h"
#include <algorithm>
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include "window-basic-main-outputs.hpp"

class SourceManager
{
	typedef struct _AudioCallbackData
	{
		SourceManager *manager;
		obs_source_t *source;
		obs_volmeter_t *volmeter;
	}SAudioCallbackData,*PSAudioCallbackData;
public:
	typedef struct _SourceRenderInfo
	{
		std::string source_name;
		gs_rect rect;
		gs_window window;
		obs_display_t *display;
	}SourceRenderInfo;
public:
    SourceManager();
    ~SourceManager();
    int init();

    void createPreview(std::shared_ptr<SourceRenderInfo> info);
    int initConfig();
    void InitHotkeys();
    bool ResetAudio();
    int ResetVideo();
    void InitOBSCallbacks();
    void ResetOutputs();
    bool InitService();
    bool updateService(std::string server,std::string key);
    void InitPrimitives();
    bool StreamingActive();

	//
	int stopStream(bool bForce = false);
	void startStream();

	// 
	void startRecord();
	void stopRecord();
    //void StreamingStop(int code);
    void StreamingStart();

    void ResetAudioDevice(const char *sourceId, const char *deviceName,
            const char *deviceDesc, int channel);
    static void renderingPreview(void *data, uint32_t cx, uint32_t cy);
    void DrawBackdrop(float cx, float cy);
    inline const char *Str(const char *lookup) {return lookup;}
	void ActivateAudioSource(const char* srcName);
	void DeactivatAudioSource(const char* srcName);
	void addOffset(int64_t microseconds);
	// return milliseconds
	uint64_t durationOfLive();

	void offsetStart(int64_t microseconds);
	void offsetStop();
	void firstVideoFrameEncodePrepare(uint64_t timestamp);

	void resetReconnectOffset();
	void resetOffsetForNextRecording();
public:
    const char* getLogoSourceName();
    inline config_t * Config() const {return basicConfig;}
	void saveBaseConfig() 
	{
		basicConfig.Save();
	}
    config_t * globalConfig() const;
    obs_service_t* GetService();
//    static void SourceAdded(void *data, calldata_t *params);
    static void SourceRemoved(void *data, calldata_t *params);
//    static void SourceLoaded(void *data, obs_source_t *source);
    static void ChannelChanged(void *data, calldata_t *params);
    static void SourceActivated(void *data, calldata_t *params);
    static void SourceDeactivated(void *data, calldata_t *params);
    static void SourceRenamed(void *data, calldata_t *params);
	//static void OBSVolumeLevel(void *data, float level, float mag,
	//	float peak, float muted);
	static void OBSVolumeLevel(void *data,
		const float magnitude[MAX_AUDIO_CHANNELS],
		const float peak[MAX_AUDIO_CHANNELS],
		const float inputPeak[MAX_AUDIO_CHANNELS]);

public:
	int hostMode;
	bool isMainSpeaker;
    float devicePixelRatio(){return dPixelRatio;}

    void removePreview(const std::string &source_name);

    ////////////
    int screenWidth;
    int screenHeight;
	void requestRedirectStream();
	void notifyReconnectStream();
	void notifyReconnectStreamSuccess();
	void notifyVolume(const char* srcName, float level, float mag, float peak);
	void removeVolmeter(const char *id, const char * srcName);
	void setServerRecordPauseStatus(bool pause);
private:
    friend class CCEngine;
    friend class ScenePreview;
    friend class OBSBasicStatusBar;
	friend class VideoEncodeParamForH264;
    ConfigFile basicConfig;
    OBSService m_service;
    //obs_source_t *currentSource= nullptr;

	std::map<const char *, SAudioCallbackData  *> volmeterMap;

    std::unique_ptr<BasicOutputHandler> outputHandler= nullptr;
//    BasicOutputHandler *outputHandler;

	
	std::atomic<uint64_t>  m_offset;
	std::atomic<uint64_t> m_start_time_point_ns;
	std::atomic<uint64_t> m_first_video_frame_ns;
	std::atomic<uint64_t> m_reconnect_time_point_ns;

	uint64_t m_record_offset = 0;
	std::atomic<uint64_t> m_pause_recored_start_timepoint;

    gs_vertbuffer_t *box = nullptr;
    gs_vertbuffer_t *boxLeft = nullptr;
    gs_vertbuffer_t *boxTop = nullptr;
    gs_vertbuffer_t *boxRight = nullptr;
    gs_vertbuffer_t *boxBottom = nullptr;
    gs_vertbuffer_t *circle = nullptr;
    float dPixelRatio;

	std::atomic<bool> reconnecting;

	std::map<std::string, std::shared_ptr<SourceRenderInfo>> m_render_info_list;
};

#endif // SOURCEMANAGER_H_
