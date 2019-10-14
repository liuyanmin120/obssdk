#ifndef AudioInputSourcePropertyImpl_h_
#define AudioInputSourcePropertyImpl_h_

#include "IBroadcastEngine.h"
#include "obs.h"

#include <map>
#include <string>

class AudioSourcePropertyImpl : public IAudioSourceProperty
{
public:
	AudioSourcePropertyImpl(const char *src_name);
	~AudioSourcePropertyImpl();

protected:
	Broadcast::ESourceType type() override;
	const char * id() override;
	const char * name() override;
	void update() override;

	void enumAudioDevice(std::function<bool(const char *name, const char *id)> callback) override;
	bool setAudioDevice(const char *device_id, bool update_now) override;
	void currentAudioDevice(std::function<void(const char *name, const char *id)> callback) override;

	virtual int getAudioVolume() override;
	virtual int setAudioVolume(int vol)override;

	void setAudioPhaseReverse(bool reverse);

	virtual int setAudioMuted(bool bMuted)override;
	virtual bool getAudioMuted() override;
private:
	std::string m_source_name;
	
	obs_properties_t *m_props;
};


#endif // AudioInputSourcePropertyImpl_h_
