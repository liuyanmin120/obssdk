#ifndef VideoPropertyImpl_h__
#define VideoPropertyImpl_h__

#include "IBroadcastEngine.h"
#include "obs.h"

#include <map>
#include <string>

class VideoSourcePropertyImpl : public IVideoSourceProperty
{
public:
	VideoSourcePropertyImpl(const char *src_name, bool create_source);
	~VideoSourcePropertyImpl();

protected:
	Broadcast::ESourceType type() override;
	const char * id() override;
	const char * name() override;
	void update() override;

	void enumVideoDevice(std::function<bool(const char *name, const char *id)> callback) override;
	bool setVideoDevice(const char *device_id, bool update_now) override;
	void currentVideoDevice(std::function<void(const char *name, const char *id)> callback) override;

	void enumCustomAudioDevice(std::function<bool(const char *name, const char *id)> callback) override;
	void setAudioDevice(const char *device_id) override;
	void currentAudioDevice(std::function<void(const char *name, const char *id)> callback) override;

	void setAudioOutputMode(AudioOutputMode mode) override;
	AudioOutputMode audioOutputMode() override;

	void setCustomAudioMode(bool use) override;
	bool isCustomAudioMode() override;

	void setActive(bool active) override;
	void configuration(long long opaque) override;

	void setCustomMode(bool use) override;
	bool isCustomMode() override;

	void enumResolutions(std::function<bool(const char *resolution)> callback) override;
	void setResolution(const char *resolution) override;
	const char * currentResolution() override;

	void enumVideoFormat(std::function<bool(const char *format, int32_t index)> callback) override;
	void setVideoFormat(int32_t index) override;
	const char *getVideoFormat() override;
private:
	void createVideoSource();

private:
	std::string m_source_name;
	std::string m_current_camera_name;
	
	obs_properties_t *m_props;
};


#endif // VideoPropertyImpl_h__
