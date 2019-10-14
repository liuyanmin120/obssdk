#include "BroadcastEngine.h"
#include <QDebug>

CBroadcastEngine *CBroadcastEngine::m_pInstance = Q_NULLPTR;

CBroadcastEngine::CBroadcastEngine(QObject *parent)
	: QObject(parent)
{
	m_pEngine = IBroadcastEngine::create(this);
}

CBroadcastEngine::~CBroadcastEngine()
{
	m_pEngine = nullptr;
}

CBroadcastEngine * CBroadcastEngine::instance()
{
	if (!m_pInstance)
	{
		m_pInstance = new CBroadcastEngine;
	}
	return m_pInstance;
}

IBroadcastEngine * CBroadcastEngine::engine()
{
	if (!m_pInstance)
		return nullptr;
	return m_pInstance->m_pEngine;
}

void CBroadcastEngine::releaseEngine()
{
	IBroadcastEngine::release(engine());
	m_pInstance->m_pEngine = nullptr;
}

void CBroadcastEngine::OnSceneInitialled()
{
	emit signalSceneInitialled();
}

void CBroadcastEngine::OnOutputSizeChange(int32_t width, int32_t height)
{
	emit signalOutputSizeChange(width, height);
}

void CBroadcastEngine::onLogMsg(const std::string &msg, int log_level)
{
	//qDebug() << "msg = " << msg.c_str();
}

void CBroadcastEngine::OnActivateAudioSource(const char * srcName)
{
	emit signalActivateAudioSource(QString::fromUtf8(srcName));
}

void CBroadcastEngine::OnDeactivatAudioSource(const char * srcName)
{
	emit signalDeactivatAudioSource(QString::fromUtf8(srcName));
}

void CBroadcastEngine::OnAudioVolume(const char * srcName, float mag, float peak, float peakHold)
{
	emit signalAudioVolume(srcName, mag, peak, peakHold);
}

void CBroadcastEngine::OnSourceSelectionChange(const char *src_name, bool selected)
{
	emit signalSourceSelectionChange(src_name, selected);
}


void CBroadcastEngine::OnCurrentSceneChange(const char *current_scene, const char *previous_scene)
{

}

void CBroadcastEngine::OnSuccessStartPublish()
{
	emit signalStartPublishSuccess();
}

void CBroadcastEngine::OnStopPublish(int code, const char * msg)
{
	QString qMsg;
	if (msg) 
	{
		qMsg = msg;
	}
	emit signalStopPublish(code, qMsg);
}

void CBroadcastEngine::OnReconnectStream() 
{
	emit signalReconnectStream();
}

void CBroadcastEngine::OnReconnectStreamSuccess() 
{
	emit signalReconnectStreamSuccess();
}

void CBroadcastEngine::OnSuccessStartRecord() 
{
	emit signalStartRecordSuccess();
}

void CBroadcastEngine::OnStopRecord(int code, const char *msg) 
{
	QString qMsg;
	if (msg)
	{
		qMsg = msg;
	}
	emit signalStopRecord(code, qMsg);
}