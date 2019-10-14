#ifndef BroadcastEngine_h__
#define BroadcastEngine_h__

#ifdef WIN32
#ifdef BROADCAST_ENGINE_EXPORT
#define BROADCAST_ENGINE_API __declspec(dllexport)
#else
#define BROADCAST_ENGINE_API
#endif
#else
#define BROADCAST_ENGINE_API
#endif

#ifndef WIN32
typedef unsigned long size_t;
#endif

#ifdef WIN32
#define VIDEOSOURCEID "dshow_input"
/* In windows platform ,the input audio plugin is changed ,   
#define AUDIOINPUT  "dshow_input_audio"
*/
#define AUDIOINPUT  "wasapi_input_capture"

#define AUDIOOUT  "wasapi_output_capture"
#define DISPLAYCAPTURE "monitor_capture"
#define VIDEO_DEVICE_ID "video_device_id"
#define MONITOR_ENUM "monitor"
#else
#define VIDEOSOURCEID "av_capture_input"
#define AUDIOINPUT  "coreaudio_input_capture"
#define AUDIOOUT  "coreaudio_output_capture"
#define DISPLAYCAPTURE "display_capture"
#define VIDEO_DEVICE_ID "device"
#define MONITOR_ENUM "display"
#endif
#define DEFAULT_SCENE "default_scene"

#define AUDIO_SOURCE_FLAG 0x01
#define VIDEO_SOURCE_FLAG 0x02

#include <functional>
#include <memory>
#include <vector>
#include <string>

#define STANDARD_AUDIO_INPUT_DEVICE "standard_audio_input_device"
#define STANDARD_AUDIO_OUTPUT_DEVICE "standard_audio_output_device"

namespace Broadcast
{
	enum ESourceType
	{
		VIDEO_CAPTURE_SOURCE,
		IMAGE_FILE_SOURCE,
		MONITOR_CAPTURE_SOURCE,
		WINDOW_CAPTURE_SOURCE,
		AUDIO_INPUT_SOURCE,
		AUDIO_OUTPUT_SOURCE,
		AREA_CAPTURE_SOURCE,
		NONE_SOURCE,
	};

	struct SRect
	{
		int x;
		int y;
		int width;
		int height;
	};

	enum  EMouseButton 
	{
		MOUSE_LEFT,
		MOUSE_RIGHT,
	};

	enum EKey 
	{
		KEY_NONE = 0x00,
		KEY_SHIFT = 0x02,
		KEY_CONTROL = 0x04,
		KEY_ALT = 0x08,
	};

	struct SMouseEvent 
	{
		EMouseButton button;
		EKey         modifiers;
		int           x;
		int           y;
	};

	enum EInitSdkResult 
	{
		INITIAL_OK,
		OPEN_GLOBAL_CONFIG_ERROR = -1,
		CORE_MODULE_INITIAL_ERROR = -2,
		AUDIO_INITIAL_ERROR = -3,
		VIDEO_MODULE_NOT_FOUND_ERROR = -4,
		VIDEO_NOT_SUPPORTED_ERROR = -5,
		VIDEO_INVALID_PARAM_ERROR = -6,
		SEVERICE_INTINAL_ERROR = -7,
		UNSPECIFIED_ERROR = -8
	};

	//Դ����Ⱦ����Ĳ㼶�ƶ��ʹ��ڱ任
	enum  ESourceActiton {
		//�ƶ��㼶
		ORDER_MOVE_UP,
		ORDER_MOVE_DOWN,
		ORDER_MOVE_TOP,
		ORDER_MOVE_BOTTOM,
		//���ڱ任
		RESET_TRANSFORM,
		ROTATE_90,
		ROTATE_180,
		ROTATE_270,
		FLIP_HORIZONTAL,
		FLIP_VERTICAL,
		FIT_TO_SCREEN,
		STRETCH_TO_SCREEN,
		CENTER_TO_SCREEN,
	};

	enum EStopStreamCode 
	{
		STOP_STREAM_SUCCESS = 0,
		STOP_STREAM_BAD_PATH = -1,
		STOP_STREAM_CONNECT_FAILED = -2,
		STOP_STREAM_INVALID_STREAM = -3,
		STOP_STREAM_ERROR = -4,
		STOP_STREAM_DISCONNECTED = -5,
		STOP_STREAM_UNSUPPORTED = -6,
		STOP_STREAM_NO_SPACE = -7,
	};
}

class ISceneManager;
class BROADCAST_ENGINE_API IBroadcastEngineObserver
{
public:
	//������Ƶ�豸
	virtual void OnActivateAudioSource(const char* srcName) = 0;
	//ȡ������
	virtual void OnDeactivatAudioSource(const char* srcName) = 0;
	//OBS ��˷�/������ֵ���µĻص�
	virtual void OnAudioVolume(const char* srcName, float mag, float peak, float peakHold) = 0;
	//����Դѡ�����ѡ��
	virtual void OnSourceSelectionChange(const char *src_name, bool selected) = 0;
	//������ʼ����ɻص�
	virtual void OnSceneInitialled() = 0;
	//����ֱ����л�֪ͨ
	virtual void OnOutputSizeChange(int32_t width, int32_t height) = 0;
	// �����л��ص�֪ͨ
	virtual void OnCurrentSceneChange(const char *current_scene, const char *previous_scene) = 0;
	// ��־�ص�
	virtual void onLogMsg(const std::string &, int log_level) = 0;
   
	/**************** ���½ӿ�Ϊ������ػص� ******************/
	//�ɹ������ص�
	virtual void OnSuccessStartPublish() = 0;
	//����ֹͣ����ʧ�ܻص�
	virtual void OnStopPublish(int code,const char *msg) = 0;
	//OBS�����Ļص�����
	virtual void OnReconnectStream() = 0;
	//OBS�����ɹ��Ļص�����
	virtual void OnReconnectStreamSuccess() = 0;
	//OBS ¼�Ƴɹ�
	virtual void OnSuccessStartRecord() = 0;
	//OBS ¼��ʧ�ܻ���ֹͣ
	virtual void OnStopRecord(int code, const char *msg) = 0;
};

class BROADCAST_ENGINE_API IBroadcastEngine
{
public:
	//����ʵ������ʵ��ֻ�ܴ���һ��
	static IBroadcastEngine* create(IBroadcastEngineObserver* observer);
	//�ͷ�ʵ��
	static void release(IBroadcastEngine* object);
	//��ʼ��SDK
	virtual int init() = 0;
public:
	virtual int load() = 0; 
	virtual int save() = 0;

	//���/ɾ�� ����Դ
	virtual int addSource(Broadcast::ESourceType type,const char* srcName, const char *scene_name = DEFAULT_SCENE) = 0;
	virtual int removeSource(const char *srcName) = 0;
	
	//�ж�ĳ��/ĳ������ ����Դ�Ƿ���� 
	virtual bool hasSource(const char *srcName) = 0;
	virtual bool hasSource(Broadcast::ESourceType type) = 0;
	virtual Broadcast::ESourceType getSourceTypeByName(const char* srcName) = 0;
	virtual void enumSource(std::function<bool(Broadcast::ESourceType type, const char *src_name)> callback) = 0;
	virtual int manipulateSource(const char *srcName, Broadcast::ESourceActiton operation) = 0;

	//��ʼ/ֹͣԤ�� ������Ƶ����ԴԤ��
	virtual bool startPreviewSource(const char *srcName, const void* Window) = 0;
	virtual bool stopPreviewSource(const char *srcName) = 0;
	// ��ó���Դ������ Ԥ��������ؽӿ�
	virtual std::shared_ptr<ISceneManager> sceneManager() = 0;

	/**************** ������ؽӿ� ******************/
	// ��ʼ/�������� �����Ҫ�첽�Ȼص�֪ͨ
	virtual int startStream(const char *url, const char* key) =0;
	virtual int stopStream(bool bForce = false) = 0;
	//��������ֱ���
	virtual int getOutputSize(int &width, int &height) = 0;
	virtual int setOutputSize(int width, int height) = 0;

	/**************** ¼����ؽӿ� ******************/
	 // ��ȡ/���� ¼���ļ�����·��
	virtual int setRecordFilePath(const char *file_path) = 0;
	virtual const char* getRecordFilePath() = 0;
	// ��ȡ/���� ¼���ļ� ��ʽ
	
	virtual int setRecordFileFormat(const char *recFmt) = 0;
	virtual const char * getRecordFileFormat() = 0;
	// ��ʼ/���� ¼��
	virtual int startRecord() = 0 ;
	virtual int stopRecord() = 0;
protected:
	virtual ~IBroadcastEngine() {}
};

class BROADCAST_ENGINE_API IScenePreview
{
public:
	virtual const char* mousePressEvent(Broadcast::SMouseEvent *event) = 0;
	virtual void mouseReleaseEvent(Broadcast::SMouseEvent *event) = 0;
	virtual void mouseMoveEvent(Broadcast::SMouseEvent *event) = 0;

	virtual void bindScene(const char *scene_name) = 0;
	
	virtual void setMouseTransferEnable(bool enable) = 0;
	virtual bool getMouseTransfeEnable() = 0;

	virtual void update() = 0;
protected:
	virtual ~IScenePreview() {}
};

class BROADCAST_ENGINE_API ISceneManager
{
public:
	virtual int setCurrentSceneName(const char *scene_name) = 0;
	virtual const char * getCurrentSceneName() = 0;
	virtual int createScene(const char *scene_name) = 0;
	virtual bool hasScene(const char *scene_name) = 0;

	virtual void setItemSelection(const char *src_name, bool select) = 0;
	virtual bool getItemSelected(const char *src_name) = 0;

	virtual IScenePreview * createPreview(void *window) = 0;
	virtual void releasePreview(IScenePreview *instance) = 0;

	virtual void clearItemSelected(const char *scene_name) = 0; 

	virtual void activeAllItem(const char *scene_name) = 0;
	virtual void deactiveAllItem(const char *scene_name) = 0;

	virtual void zoomAllSceneItem(float factor) = 0;

	virtual void enumSceneItem(const char *scene_name
		, std::function<bool(Broadcast::ESourceType type, const char *source_name) > callback) = 0;
	virtual void enumScene(std::function<bool(const char *scene_name)> callback) = 0;
protected:
	virtual ~ISceneManager() {}
};

class BROADCAST_ENGINE_API ISourceProperty
{
public:
	ISourceProperty() {}
public:
	virtual Broadcast::ESourceType type() = 0;
	virtual const char * id() = 0;
	virtual const char * name() = 0;
	virtual void update() = 0;
protected:
	virtual ~ISourceProperty() {}
};

class BROADCAST_ENGINE_API IAudioSourceProperty : public ISourceProperty
{
public:
	enum AudioOutputMode
	{
		Capture,
		DirectSound = 1,
		WaveOut,
	};
	static std::shared_ptr<IAudioSourceProperty> create(const char *src_name);

	virtual void enumAudioDevice(std::function<bool(const char *name, const char *id)> callback) = 0;
	virtual bool setAudioDevice(const char *id, bool update_now) = 0;
	virtual void currentAudioDevice(std::function<void(const char *name, const char *id)> callback) = 0;
	virtual int getAudioVolume() = 0;
	virtual int setAudioVolume(int vol) = 0;

	virtual int setAudioMuted(bool bMuted) = 0;
	virtual bool getAudioMuted() = 0;

protected:
	virtual ~IAudioSourceProperty() {}
};

class BROADCAST_ENGINE_API IVideoSourceProperty : public ISourceProperty
{
public:
	enum AudioOutputMode
	{
		Capture,
		DirectSound = 1,
		WaveOut,
	};
	static std::shared_ptr<IVideoSourceProperty> create(const char *src_name, bool create_source = true);

	virtual void enumVideoDevice(std::function<bool(const char *name, const char *id)> callback) = 0;
	virtual bool setVideoDevice(const char *id, bool update_now) = 0;
	virtual void currentVideoDevice(std::function<void(const char *name, const char *id)> callback) = 0;

	virtual void enumCustomAudioDevice(std::function<bool(const char *name, const char *id)> callback) = 0;
	virtual void setAudioDevice(const char *id) = 0;
	virtual void currentAudioDevice(std::function<void(const char *name, const char *id)> callback) = 0;

	virtual void setAudioOutputMode(AudioOutputMode mode) = 0;
	virtual AudioOutputMode audioOutputMode() = 0;

	virtual void setCustomAudioMode(bool use) = 0;
	virtual bool isCustomAudioMode() = 0;

	virtual void setActive(bool active) = 0;
	virtual void configuration(long long opaque = 0) = 0;

	virtual void setCustomMode(bool use) = 0;
	virtual bool isCustomMode() = 0;

	//resolution format: %dx%d
	virtual void enumResolutions(std::function<bool(const char *resolution)> callback) = 0;
	virtual void setResolution(const char *resolution) = 0;
	virtual const char * currentResolution() = 0;

	virtual void enumVideoFormat(std::function<bool(const char *format, int32_t index)> callback) = 0;
	virtual void setVideoFormat(int32_t index) = 0;
	virtual const char *getVideoFormat() = 0;
protected:
	virtual ~IVideoSourceProperty() {}
};

class BROADCAST_ENGINE_API WindowCaptureSourceProperty : public ISourceProperty
{
public:
	virtual int setCursorVisible(bool isShow) = 0;
	virtual bool getCursorVisibility() = 0;
	virtual bool getCompatibility() = 0;
	virtual int setCompatibility(bool bCompatibility) = 0;
};

class BROADCAST_ENGINE_API IAreaCaptureSourceProperty : public WindowCaptureSourceProperty
{
public:
	//���򲶻����� ������ؽӿ�
	static std::shared_ptr<IAreaCaptureSourceProperty> create(const char *src_name, bool create_source = true);
	virtual int setAreaCaptureParam(const Broadcast::SRect &rect) = 0;
	virtual bool getAreaCaptureParam(Broadcast::SRect &rect) = 0;
};

class BROADCAST_ENGINE_API IAppWindowCaptureSourceProperty : public WindowCaptureSourceProperty
{
public:
	//Ӧ�ó��򴰿�����Դ ������ؽӿ�

	static std::shared_ptr<IAppWindowCaptureSourceProperty> create(const char *src_name, bool create_source = true);
	virtual void enumAppWindows(std::function<bool(const char *name, const char *path)> callback) = 0;
	virtual int setAppWindow(const char *window) = 0;
	virtual int setAppWindow(void *hwnd) = 0;
	virtual const char* getAppWindow() = 0;

};

class BROADCAST_ENGINE_API IMonitorCaptureSourceProperty :public WindowCaptureSourceProperty
{
public:
	static std::shared_ptr<IMonitorCaptureSourceProperty> create(const char *src_name, bool create_source = true);
	//���湲������Դ ������ؽӿ�
	virtual const char* enumMonitors(size_t idx) = 0;
	virtual int setMonitor(const char *monitor) = 0;
	virtual const char* getMonitor() = 0;
};

#endif // BroadcastEngine_h__
