#ifndef BROADCASTENGINE_H
#define BROADCASTENGINE_H

#include <QObject>
#include "broadcast-engine/sources/IBroadcastEngine.h"

class CBroadcastEngine : public QObject, public IBroadcastEngineObserver
{
	Q_OBJECT

public:
	CBroadcastEngine(QObject *parent = 0);
	~CBroadcastEngine();
public:
	static CBroadcastEngine *instance();
	static IBroadcastEngine *engine();
	static void releaseEngine();
signals:
	void signalSceneInitialled();
	void signalLogin(int result, const QString &strLoginData);
	void signalStartPublishSuccess();
	void signalStopPublish(int result,const QString &msg);
	void signalStartRecordSuccess();
	void signalStopRecord(int result, const QString &msg);

	void signalCreateLiveIdNotify(const QString& liveid, const QString &strResData);
	void signalGetPitIdResult(const QString& liveid, const QString &strResData);
	void signalRtmpAllNodeTestFinish(const QString& nodeIP
		, int rtmpConnectTime, float bandwidth, int lost, int mark, int cacheCount, int notRecommend, bool isMaster);
	void signalActivateAudioSource(const QString& srcName);
	void signalDeactivatAudioSource(const QString& srcName);
	void signalReconnectStream();
	void signalReconnectStreamSuccess();
	void signalAudioVolume(const char* srcName, float mag, float peak, float peakHold);
	void signalThroughputAdaptation(float predicted, int band_width, int bit_rate);
	void signalImmediateBitrate(int bitrate);
	void signalStreamTimeout();

	void signalSourceSelectionChange(const QString &src_name, bool selected);
	void signalCurrentSceneChange(const QString &current_scene, const QString &previous_scene);
	void signalSourceAdded(const QString &src_name, int type);
	void signalSourceRemoved(const QString &src_name, int type);

	void signalOutputSizeChange(int width, int height);
private:
	//激活音频设备
	virtual void OnActivateAudioSource(const char* srcName) override;
	//取消激活
	virtual void OnDeactivatAudioSource(const char* srcName) override;
	//OBS 麦克风/音量条值更新的回调
	virtual void OnAudioVolume(const char* srcName, float mag, float peak, float peakHold) override;
	//数据源选择与非选中
	virtual void OnSourceSelectionChange(const char *src_name, bool selected) override;
	//场景初始化完成回调
	virtual void OnSceneInitialled() override;
	//输出分辨率切换通知
	virtual void OnOutputSizeChange(int32_t width, int32_t height) override;
	virtual void onLogMsg(const std::string &, int log_level) override;
	virtual void OnCurrentSceneChange(const char *current_scene, const char *previous_scene)override;

	//成功推流回调
	virtual void OnSuccessStartPublish() override;
	//成功推流回调
	virtual void OnStopPublish(int code, const char *msg) override;
	//OBS重连的回调函数
	virtual void OnReconnectStream() override;
	//OBS重连成功的回调函数
	virtual void OnReconnectStreamSuccess() override;

	//OBS 录制成功
	virtual void OnSuccessStartRecord() override;
	//OBS 录制失败或者停止
	virtual void OnStopRecord(int code, const char *msg) override;
private:
	static CBroadcastEngine *m_pInstance;
	IBroadcastEngine *m_pEngine;
};

#endif // BROADCASTENGINE_H
