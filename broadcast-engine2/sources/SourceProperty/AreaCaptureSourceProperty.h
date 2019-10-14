#ifndef AREA_CAPTURE_SOURCE_POPERTY_H
#define AREA_CAPTURE_SOURCE_POPERTY_H

#include "IBroadcastEngine.h"
#include "obs.hpp"

class AreaCaptureSourcePropertyImpl :
	public IAreaCaptureSourceProperty
{
public:
	AreaCaptureSourcePropertyImpl(const char *src_name, bool create_source);
	virtual ~AreaCaptureSourcePropertyImpl();

	// ISourceProperty impl
	virtual Broadcast::ESourceType type() override;
	virtual const char * id() override;
	virtual const char * name() override;
	virtual void update() override;
	virtual int setAreaCaptureParam(const Broadcast::SRect &rect) override;
	virtual bool getAreaCaptureParam(Broadcast::SRect &rect) override;

	// WindowCaptureSourcePropery impl
	virtual int setCursorVisible(bool isShow) override;
	virtual bool getCursorVisibility() override;
	virtual bool getCompatibility() override;
	virtual int setCompatibility(bool bCompatibility) override;
private:
	std::string			m_sourceName;
	obs_properties_t *	m_props = nullptr;
};

#endif //AREA_CAPTURE_SOURCE_POPERTY_H