#pragma once
#include "IBroadcastEngine.h"
#include "obs.hpp"

class AppWindowCaptureSourcePropertyImpl :
	public IAppWindowCaptureSourceProperty
{
public:
	AppWindowCaptureSourcePropertyImpl(const char *src_name, bool create_source);
	~AppWindowCaptureSourcePropertyImpl();

// ISourceProperty impl
	virtual Broadcast::ESourceType type() override;
	virtual const char * id() override;
	virtual const char * name() override;
	virtual void update() override;

// IWindowCaptureSourcePropery impl
	virtual int setCursorVisible(bool isShow) override;
	virtual bool getCursorVisibility() override;
	virtual bool getCompatibility() override;
	virtual int setCompatibility(bool bCompatibility) override;

//IAppWindowCaptureSourceProperty impl
	virtual void enumAppWindows(std::function<bool(const char *name, const char *path)> callback) override;
	virtual int setAppWindow(const char *window) override;
	virtual int setAppWindow(void *hwnd) override;
	virtual const char* getAppWindow() override;
private :
	int createAppWindowSource();
private :
	std::string				m_sourceName;
	obs_properties_t		*m_props;
	void *					m_hwnd = nullptr;
};

