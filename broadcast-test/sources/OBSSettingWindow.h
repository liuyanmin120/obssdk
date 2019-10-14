#pragma once

#include <QtWidgets/QMainWindow>
#include <memory>
#include "ui_OBSSettingWindow.h"
#include "BroadcastEngine.h"

enum PublishStatus 
{
	PS_Stopped,
	PS_Starting,
	PS_Started,
	PS_Stopping,
};
class CAreaCaptureFrame;
class OBSSettingWindow : public QMainWindow
{
	Q_OBJECT

public:
	OBSSettingWindow(QWidget *parent = Q_NULLPTR);
	~OBSSettingWindow();
protected:
	void closeEvent(QCloseEvent * event) override;
	bool eventFilter(QObject *watched, QEvent *event)override;
private:
	void updateAppWindowSrcPage();
	void updateVideoSrcPage();
	void updateMonitorSrcPage();
	void updateAreaCaptureSrcPage();
	void updateStartPublishBtnStatus();
	void updateRecordBtnStatus();
private:
	void initAllSupportSource();
	void switchSource(int srcTyp);
	void initAudioInputSource();
	void initAudioOutputSource();
private slots:
void on_startPublishBtn_clicked();
void on_startRecordBtn_clicked();

void on_refreshInputDeviceBtn_clicked();
void on_refreshOutputDevice_clicked();
void slotOnStartPublishSuccess();
void slotOnStopPublishFinished(int code,const QString &msg);
void slotStartRecordSuccess();
void slotStopRecordFinished(int code ,const QString &msg);
private:
	Ui::OBSSettingForm ui;
	QMap<Broadcast::ESourceType, QString>	m_allSupportSource;
	Broadcast::ESourceType					m_curSrcType = Broadcast::NONE_SOURCE;
	std::shared_ptr<CAreaCaptureFrame>      m_areaCaptureFrame;
	PublishStatus							m_pubStatus = PS_Stopped;
	bool									m_isRecording = false;
	QWidget									*m_mainWindow;
	int										m_bInit = false;
	PublishStatus							m_recordStatus = PS_Stopped;
};
