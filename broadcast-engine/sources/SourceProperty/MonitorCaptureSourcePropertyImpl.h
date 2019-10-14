#pragma once
#include "IBroadcastEngine.h"
#include "obs.hpp"

class MonitorCaptureSourcePropertyImpl :
	public IMonitorCaptureSourceProperty
{
public:
	MonitorCaptureSourcePropertyImpl(const char *src_name, bool create_source);
	virtual ~MonitorCaptureSourcePropertyImpl();

	// ISourceProperty impl
	virtual Broadcast::ESourceType type() override;
	virtual const char * id() override;
	virtual const char * name() override;
	virtual void update() override;

	// WindowCaptureSourcePropery impl
	virtual int setCursorVisible(bool isShow) override;
	virtual bool getCursorVisibility() override;
	virtual bool getCompatibility() override;
	virtual int setCompatibility(bool bCompatibility) override;

	//IMonitorCaptureSourceProperty impl
	virtual const char* enumMonitors(size_t idx) override;
	virtual int setMonitor( const char *monitor) override;
	virtual const char* getMonitor() override;
private:
	std::string  m_sourceName;
	obs_properties_t *m_props;
};

