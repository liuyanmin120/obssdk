#include "OBSSettingWindow.h"
#include <QTimer>
#include "AreaCaptureFrame.h"
#include <QMessageBox>
OBSSettingWindow::OBSSettingWindow(QWidget *parent)
	: QMainWindow(nullptr),m_mainWindow(parent)
{
	ui.setupUi(this);

	ui.micVolumeBeatCtrl->setSrcName(STANDARD_AUDIO_INPUT_DEVICE);
	ui.speakerVolumeBeatCtrl->setSrcName(STANDARD_AUDIO_OUTPUT_DEVICE);

	CBroadcastEngine::instance();
	// load audio/video source
	CBroadcastEngine::engine()->load();
	// create default scene
	auto scene_manager = CBroadcastEngine::engine()->sceneManager();
	if (!scene_manager->hasScene(DEFAULT_SCENE))
	{
		scene_manager->createScene(DEFAULT_SCENE);
	}
	scene_manager->setCurrentSceneName(DEFAULT_SCENE);


	// output size  is equal to base size in default
	CBroadcastEngine::engine()->setOutputSize(320, 240);

	CBroadcastEngine::engine()->addSource(Broadcast::AUDIO_INPUT_SOURCE,STANDARD_AUDIO_INPUT_DEVICE );

	CBroadcastEngine::engine()->addSource(Broadcast::AUDIO_OUTPUT_SOURCE, STANDARD_AUDIO_OUTPUT_DEVICE);

	initAllSupportSource();
	auto ins = CBroadcastEngine::instance();
	connect(ins,&CBroadcastEngine::signalStartPublishSuccess,this,&OBSSettingWindow::slotOnStartPublishSuccess);
	connect(ins, &CBroadcastEngine::signalStopPublish, this, &OBSSettingWindow::slotOnStopPublishFinished);

	connect(ins, &CBroadcastEngine::signalStartRecordSuccess, this, &OBSSettingWindow::slotStartRecordSuccess,Qt::QueuedConnection);
	connect(ins, &CBroadcastEngine::signalStopRecord, this, &OBSSettingWindow::slotStopRecordFinished, Qt::QueuedConnection);
	if (m_mainWindow) 
	{
		m_mainWindow->installEventFilter(this);
	}
}

OBSSettingWindow::~OBSSettingWindow()
{
	CBroadcastEngine::engine()->save();
	CBroadcastEngine::releaseEngine();
}
bool OBSSettingWindow::eventFilter(QObject *watched, QEvent *event) 
{
	if (watched == m_mainWindow && event->type() == QEvent::Show) 
	{
		if (m_bInit == false) 
		{
			m_bInit = true;
			QTimer::singleShot(100, this, [this] 
			{
				ui.allSrcCombox->setCurrentIndex(2);
				//switchSource(Broadcast::WINDOW_CAPTURE_SOURCE);
			});
		}
	}
	return false;
}
void OBSSettingWindow::closeEvent(QCloseEvent * ) 
{

}

void OBSSettingWindow::updateAppWindowSrcPage()
{
	ui.stackedWidget->setCurrentWidget(ui.appWndSrcPage);
	auto appWindowProperty = IAppWindowCaptureSourceProperty::create(m_allSupportSource[m_curSrcType].toUtf8());
	ui.appComboBox->blockSignals(true);
	ui.appComboBox->disconnect();
	ui.appComboBox->clear();
	
	//m_appWindowProperty.reset();
	QString curWindow = "kooWeTea.exe";
	int nCurrentIndex = 0;
	int nSelectedIndex = 0;

	//ui.appComboBox->addItem("self", "self");

	appWindowProperty->enumAppWindows([&](const char *name, const char *path)->bool
	{

		QString titleName = QString::fromUtf8(name);
		QString strPath = QString::fromUtf8(path);
		if (titleName.contains(curWindow))
		{
			nSelectedIndex = nCurrentIndex;
		}

		ui.appComboBox->addItem(titleName, titleName);

		nCurrentIndex++;
		return true;
	});

	connect(ui.appComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int)
	{
		auto currentName = ui.appComboBox->currentData().toString();

		if (appWindowProperty)
		{
			appWindowProperty->setAppWindow(currentName.toUtf8());
		}
		//CBroadcastEngine::engine()->manipulateSource(WIN_CAPTURE_SOURCE_NAME, Broadcast::FIT_TO_SCREEN);
	});

	ui.appComboBox->setCurrentIndex(nSelectedIndex);

	auto currentName = ui.appComboBox->currentData().toString();
	appWindowProperty->setAppWindow(currentName.toUtf8());

	ui.appComboBox->blockSignals(false);
	//capture cursor
	ui.appCaptureCursorChk->disconnect();
	auto bChecked = appWindowProperty->getCursorVisibility();
	ui.appCaptureCursorChk->setChecked(bChecked);
	ui.appCaptureCursorChk->clicked(bChecked);
	connect(ui.appCaptureCursorChk, &QCheckBox::stateChanged, this, [=] (int checked)
	{
		appWindowProperty->setCursorVisible(checked);
	});

	// compatibility
	ui.appCompatibilityChk->disconnect();
	bChecked = appWindowProperty->getCompatibility();
	ui.appCompatibilityChk->setChecked(bChecked);
	ui.appCompatibilityChk->clicked(bChecked);
	connect(ui.appCompatibilityChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		appWindowProperty->setCompatibility(checked);
	});
}

void OBSSettingWindow::updateVideoSrcPage()
{
	ui.stackedWidget->setCurrentWidget(ui.videoSrcPage);
	ui.deviceComboBox->disconnect();
	ui.deviceComboBox->clear();

	auto videoPropery = IVideoSourceProperty::create(m_allSupportSource[m_curSrcType].toUtf8());
	QString currentVideoId;
	QString currentVideoName;

	videoPropery->currentVideoDevice([&](const char *name, const char *id)
	{
		currentVideoName = QString::fromUtf8(name);
		currentVideoId = QString::fromUtf8(id);
	});

	int nIndex = 0;
	int nSelectedIndex = 0;
	videoPropery->enumVideoDevice([&](const char *name,const char *id)->bool
	{
		QString deviceName = QString::fromUtf8(name);
		QString deviceID = QString::fromUtf8(id);
		ui.deviceComboBox->addItem(deviceName, deviceID);
		if (currentVideoId == deviceID) 
		{
			nSelectedIndex = nIndex;
		}
		nIndex++;
		return true;
	});
	connect(ui.deviceComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int)
	{
		auto currentDevice = ui.deviceComboBox->currentData().toString();
		videoPropery->setVideoDevice(currentDevice.toUtf8(), true);
	});

	ui.deviceComboBox->setCurrentIndex(nSelectedIndex);
}

void OBSSettingWindow::updateMonitorSrcPage()
{
	ui.stackedWidget->setCurrentWidget(ui.monitorSrcPage);

	auto monitorProperty = IMonitorCaptureSourceProperty::create(m_allSupportSource[m_curSrcType].toUtf8());
	//capture cursor
	ui.monitorCaptureCursorChk->disconnect();
	auto bChecked = monitorProperty->getCursorVisibility();
	ui.monitorCaptureCursorChk->setChecked(bChecked);
	ui.monitorCaptureCursorChk->clicked(bChecked);
	connect(ui.monitorCaptureCursorChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		monitorProperty->setCursorVisible(checked);
	});

	// compatibility
	ui.monitorCompatibilityChk->disconnect();
	bChecked = monitorProperty->getCompatibility();
	ui.monitorCompatibilityChk->setChecked(bChecked);
	ui.monitorCompatibilityChk->clicked(bChecked);
	connect(ui.monitorCompatibilityChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		monitorProperty->setCompatibility(checked);
	});
}

void OBSSettingWindow::updateAreaCaptureSrcPage()
{
	ui.stackedWidget->setCurrentWidget(ui.areaCapturePage);
	auto areaCaptureProperty = IAreaCaptureSourceProperty::create(m_allSupportSource[m_curSrcType].toUtf8());

	//capture cursor
	ui.areaCaptureCursorChk->disconnect();
	auto bChecked = areaCaptureProperty->getCursorVisibility();
	ui.areaCaptureCursorChk->setChecked(bChecked);
	ui.areaCaptureCursorChk->clicked(bChecked);
	connect(ui.areaCaptureCursorChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		areaCaptureProperty->setCursorVisible(checked);
	});

	// compatibility
	ui.areaCaptureCompatibilityChk->disconnect();
	bChecked = areaCaptureProperty->getCompatibility();
	ui.areaCaptureCompatibilityChk->setChecked(bChecked);
	ui.areaCaptureCompatibilityChk->clicked(bChecked);
	connect(ui.areaCaptureCompatibilityChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		areaCaptureProperty->setCompatibility(checked);
	});

	if (m_areaCaptureFrame == nullptr) 
	{
		m_areaCaptureFrame = std::make_shared<CAreaCaptureFrame>();
	}
	m_areaCaptureFrame->init(areaCaptureProperty);
	m_areaCaptureFrame->show();
}

void OBSSettingWindow::updateStartPublishBtnStatus()
{
	bool bEnable = true;
	QString showText;
	switch (m_pubStatus) 
	{
	case PS_Started:
		showText = QString::fromLocal8Bit("停止推流");
		break;
	case PS_Starting:
	{
		showText = QString::fromLocal8Bit("开始推流...");
		bEnable = false;
		break;
	}

	case PS_Stopped:
		showText = QString::fromLocal8Bit("开始推流");
		break;
	case PS_Stopping:
	{
		showText = QString::fromLocal8Bit("停止推流...");
		bEnable = false;
		break;
	}
	};
	ui.startPublishBtn->setEnabled(bEnable);
	ui.startPublishBtn->setText(showText);
}

void OBSSettingWindow::updateRecordBtnStatus() 
{
	bool bEnable = true;
	QString showText;
	switch (m_recordStatus)
	{
	case PS_Started:
		showText = QString::fromLocal8Bit("停止录制");
		break;
	case PS_Starting:
	{
		showText = QString::fromLocal8Bit("开始录制...");
		bEnable = false;
		break;
	}

	case PS_Stopped:
		showText = QString::fromLocal8Bit("开始录制");
		break;
	case PS_Stopping:
	{
		showText = QString::fromLocal8Bit("停止录制...");
		bEnable = false;
		break;
	}
	};

	ui.startRecordBtn->setEnabled(bEnable);
	ui.startRecordBtn->setText(showText);
}

void OBSSettingWindow::switchSource(int nType)
{
	auto srcType = static_cast<Broadcast::ESourceType>(nType);
	if (srcType != m_curSrcType && m_curSrcType != Broadcast::NONE_SOURCE)
	{
		CBroadcastEngine::engine()->removeSource(m_allSupportSource[m_curSrcType].toUtf8());
	}

	if (!m_allSupportSource.contains(srcType))
	{
		return;
	}

	m_curSrcType = srcType;
	auto srcName = m_allSupportSource[m_curSrcType];
	if (!CBroadcastEngine::engine()->hasSource(srcName.toUtf8())) 
	{
		CBroadcastEngine::engine()->addSource(m_curSrcType, srcName.toUtf8());
	}
	
	CBroadcastEngine::engine()->manipulateSource(srcName.toUtf8(), Broadcast::FIT_TO_SCREEN);

	if (m_curSrcType != Broadcast::AREA_CAPTURE_SOURCE && m_areaCaptureFrame)
	{
		m_areaCaptureFrame->hide();
	}

	switch (m_curSrcType)
	{
	case Broadcast::VIDEO_CAPTURE_SOURCE:
		updateVideoSrcPage();

		break;
	case Broadcast::WINDOW_CAPTURE_SOURCE:
		updateAppWindowSrcPage();
		
		break;
	case Broadcast::MONITOR_CAPTURE_SOURCE:
		updateMonitorSrcPage();

		break;
	case Broadcast::AREA_CAPTURE_SOURCE:
		updateAreaCaptureSrcPage();
		break;
	}
}

void OBSSettingWindow::on_startRecordBtn_clicked()
{

	//m_isRecording = !m_isRecording;
	if (m_recordStatus == PS_Stopped) 
	{
		auto filePath = ui.recordSavePath->text();
		if (filePath.isEmpty())
		{
			// to do log
			return;
		}
		CBroadcastEngine::engine()->setRecordFilePath(filePath.toUtf8().data());
		auto fm = ui.recordFormatCombox->currentText();
		CBroadcastEngine::engine()->setRecordFileFormat(fm.toUtf8().data());
		CBroadcastEngine::engine()->startRecord();
		m_recordStatus = PS_Starting;
		updateRecordBtnStatus();
		//ui.startRecordBtn->setText(QString::fromLocal8Bit("停止录制"));
	}
	else if(m_recordStatus == PS_Started)
	{
		//ui.startRecordBtn->setText(QString::fromLocal8Bit("开始录制"));
		m_recordStatus = PS_Stopping;
		updateRecordBtnStatus();
		CBroadcastEngine::engine()->stopRecord();
	}
}

void OBSSettingWindow::slotOnStartPublishSuccess()
{
	m_pubStatus = PS_Started;
	updateStartPublishBtnStatus();
}

void OBSSettingWindow::slotStartRecordSuccess() 
{
	m_recordStatus = PS_Started;
	updateRecordBtnStatus();
}
void OBSSettingWindow::slotStopRecordFinished(int code, const QString &msg)
{
	if (code != Broadcast::STOP_STREAM_SUCCESS && !msg.isEmpty())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("录制失败"), msg);
	}

	m_recordStatus = PS_Stopped;
	updateRecordBtnStatus();
}

void OBSSettingWindow::slotOnStopPublishFinished(int code, const QString &msg) 
{
	const char *errorDescription;
	QString errorMessage;
	bool use_last_error = false;

	switch (code) {
	case Broadcast::STOP_STREAM_BAD_PATH:
		errorMessage = QString::fromLocal8Bit("无效的路径或URL。请检查您的设置以确认它们是有效的");
		break;

	case Broadcast::STOP_STREAM_CONNECT_FAILED:
		use_last_error = true;
		errorMessage = QString::fromLocal8Bit("无法连接到服务器");
		break;

	case Broadcast::STOP_STREAM_INVALID_STREAM:
		errorMessage = QString::fromLocal8Bit("无法访问指定的频道或串流密钥，请仔细检查您的串流密钥。如果没有问题，则可能是连接到服务器时出现问题。");
		break;

	case Broadcast::STOP_STREAM_ERROR:
		use_last_error = true;
		errorMessage = QString::fromLocal8Bit("试图连接到服务器时出现意外的错误。详细信息记录在日志文件中。");
		break;

	case Broadcast::STOP_STREAM_DISCONNECTED:
		/* doesn't happen if output is set to reconnect.  note that
		* reconnects are handled in the output, not in the UI */
		use_last_error = true;
		errorMessage = QString::fromLocal8Bit("已从服务器断开");
		break;
	}

	if (code != Broadcast::STOP_STREAM_SUCCESS &&!errorMessage.isEmpty()) 
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("推流失败"), errorMessage);
	}

	m_pubStatus = PS_Stopped;
	updateStartPublishBtnStatus();
}

void OBSSettingWindow::initAllSupportSource()
{
	ui.allSrcCombox->disconnect();
	ui.allSrcCombox->clear();
	m_allSupportSource.insert(Broadcast::VIDEO_CAPTURE_SOURCE, QString::fromLocal8Bit("摄像头"));
	m_allSupportSource.insert(Broadcast::WINDOW_CAPTURE_SOURCE, QString::fromLocal8Bit("应用程序捕获"));
	m_allSupportSource.insert(Broadcast::MONITOR_CAPTURE_SOURCE, QString::fromLocal8Bit("桌面捕获"));
	m_allSupportSource.insert(Broadcast::AREA_CAPTURE_SOURCE, QString::fromLocal8Bit("区域捕获"));

	int nSelectedIndex = 0;
	int curIndex = 0;
	for (auto srcType : m_allSupportSource.keys()) 
	{
		ui.allSrcCombox->addItem(m_allSupportSource[srcType], (int)srcType);
		if (srcType == Broadcast::VIDEO_CAPTURE_SOURCE)
		{
			nSelectedIndex = curIndex;
		}
		curIndex++;
	}
	ui.allSrcCombox->setCurrentIndex(nSelectedIndex);
	connect(ui.allSrcCombox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int)
	{
		auto srcType = ui.allSrcCombox->currentData().toInt();
		switchSource(srcType);
	});

	switchSource(Broadcast::VIDEO_CAPTURE_SOURCE);
	initAudioInputSource();
	initAudioOutputSource();
}

void OBSSettingWindow::on_startPublishBtn_clicked() 
{
	if (m_pubStatus == PS_Started) 
	{
		m_pubStatus = PS_Stopping;
		CBroadcastEngine::engine()->stopStream();
		updateStartPublishBtnStatus();
	}
	else if (m_pubStatus == PS_Stopped) 
	{
		auto rtmpUrl = ui.addressLineEdit->text();
		auto key = ui.keyLineEdit->text();
		if (rtmpUrl.isEmpty() || key.isEmpty())
		{
			return;
		}
		m_pubStatus = PS_Starting;
		CBroadcastEngine::engine()->startStream(rtmpUrl.toUtf8().data(), key.toUtf8().data());
		updateStartPublishBtnStatus();
	}
	
}

void OBSSettingWindow::initAudioInputSource() 
{
	auto audioInputPropery = IAudioSourceProperty::create(STANDARD_AUDIO_INPUT_DEVICE);
	QString currentInputAudioId;
	QString currentInputAudioName;
	ui.micComboBox->disconnect();
	ui.micComboBox->clear();
	audioInputPropery->currentAudioDevice([&](const char *name, const char *id)
	{
		currentInputAudioName = QString::fromUtf8(name);
		currentInputAudioId = QString::fromUtf8(id);
	});

	int nIndex = 0;
	int nSelectedIndex = 0;

	audioInputPropery->enumAudioDevice([&](const char *name, const char *id)->bool
	{
		QString deviceName = QString::fromUtf8(name);
		QString deviceID = QString::fromUtf8(id);
		ui.micComboBox->addItem(deviceName, deviceID);
		if (currentInputAudioId == deviceID)
		{
			nSelectedIndex = nIndex;
		}
		nIndex++;
		return true;
	});

	connect(ui.micComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int)
	{
		auto currentDeviceId = ui.micComboBox->currentData().toString();
		auto currentDeviceText = ui.micComboBox->currentText();
		audioInputPropery->setAudioDevice(currentDeviceId.toUtf8(), true);
	});

	ui.micComboBox->setCurrentIndex(nSelectedIndex);

	// slider level label
	connect(ui.micVolumeSlider, &QSlider::valueChanged, this, [=](int value)
	{
		audioInputPropery->setAudioVolume(value);
		ui.micVolumeLable->setText(QString::number(value));
	});

	ui.micVolumeSlider->setValue(audioInputPropery->getAudioVolume());
}

void OBSSettingWindow::initAudioOutputSource() 
{
	auto audioOutputPropery = IAudioSourceProperty::create(STANDARD_AUDIO_OUTPUT_DEVICE);
	QString currentOutputAudioId;
	QString currentOutputAudioName;
	ui.speakerCombox->disconnect();
	ui.speakerCombox->clear();

	audioOutputPropery->currentAudioDevice([&](const char *name, const char *id)
	{
		currentOutputAudioName = QString::fromUtf8(name);
		currentOutputAudioId = QString::fromUtf8(id);
	});

	int nIndex = 0;
	int nSelectedIndex = 0;

	audioOutputPropery->enumAudioDevice([&](const char *name, const char *id)->bool
	{
		QString deviceName = QString::fromUtf8(name);
		QString deviceID = QString::fromUtf8(id);
		ui.speakerCombox->addItem(deviceName, deviceID);
		if (currentOutputAudioId == deviceID)
		{
			nSelectedIndex = nIndex;
		}
		nIndex++;
		return true;
	});

	connect(ui.speakerCombox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [=](int)
	{
		auto currentDeviceId = ui.speakerCombox->currentData().toString();
		auto currentDeviceText = ui.speakerCombox->currentText();
		audioOutputPropery->setAudioDevice(currentDeviceId.toUtf8(), true);
	});

	ui.speakerCombox->setCurrentIndex(nSelectedIndex);


	//system audio 
	ui.captureSystemAudioChk->disconnect();
	auto bChecked = audioOutputPropery->getAudioMuted();
	ui.captureSystemAudioChk->setChecked(!bChecked);
	ui.captureSystemAudioChk->clicked(!bChecked);
	connect(ui.captureSystemAudioChk, &QCheckBox::stateChanged, this, [=](int checked)
	{
		audioOutputPropery->setAudioMuted(!checked);
	});

	// slider level label
	ui.speakerVolumeSlider->disconnect();
	connect(ui.speakerVolumeSlider, &QSlider::valueChanged, this, [=](int value)
	{
		audioOutputPropery->setAudioVolume(value);
		ui.speakerVolumeLabel->setText(QString::number(value));
	});
	
	ui.speakerVolumeSlider->setValue(audioOutputPropery->getAudioVolume());
}

void OBSSettingWindow::on_refreshInputDeviceBtn_clicked() 
{
	initAudioInputSource();
}

void OBSSettingWindow::on_refreshOutputDevice_clicked() 
{
	initAudioOutputSource();
}
