#include "MicrophoneVolumeCtrl.h"
#include <QStyleOption>
#include <QPainter>
#include <QDebug>
#include "BroadcastEngine/BroadcastEngine.h"
//#include "media_engine/MediaEngineInterface.h"

const QString micIconPath = ":/main/res/main/mic_bg.png";

const QString  selectedColor = "#26BB75";
const QString  unSelectedColor = "#25262A";

MicrophoneVolumeCtrl::MicrophoneVolumeCtrl(QWidget *p):QWidget(p)
{

	m_unSelectedColor = unSelectedColor;
}

MicrophoneVolumeCtrl::~MicrophoneVolumeCtrl()
{

}

void MicrophoneVolumeCtrl::init(int nBlockWidth, int nBlockHeight, bool drawIcon)
{
	m_nBlockWidth = nBlockWidth;
	m_nBlockHeight = nBlockHeight;
	
	m_bDrawIcon = drawIcon;
	m_unSelectedColor = "#DBDBE2";
}

void MicrophoneVolumeCtrl::resetSignals()
{
	//disconnect(m_curConnect);

	//auto pMediaEngine = IMediaEngine::instance();
	//m_curConnect = connect(pMediaEngine, &IMediaEngine::sigMicrophoneVolumeChange, this, [this](float fPercentage)
	//{
	//	m_percentage = fPercentage;
	//	update();

	//}, Qt::QueuedConnection);
	disconnect(this);
	auto ins = CBroadcastEngine::instance();
	connect(ins, &CBroadcastEngine::signalActivateAudioSource, this, [this](const QString& srcName)
	{
		if (srcName == m_srcName) 
		{
			m_srcIsActive = true;
		}
	});

	connect(ins, &CBroadcastEngine::signalDeactivatAudioSource, this, [this](const QString& srcName)
	{
		if (srcName == m_srcName)
		{
			m_srcIsActive = false;
		}
	});

	connect(ins, &CBroadcastEngine::signalAudioVolume, this, [this](const char* srcName, float mag, float peak, float peakHold)
	{
		if (QString(srcName) == m_srcName && m_srcIsActive)
		{
			m_percentage = peakHold;
			update();
		}
	});
	
}

void MicrophoneVolumeCtrl::paintEvent(QPaintEvent * event)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

	if (m_bDrawIcon) 
	{
		QPixmap pix;
		if (pix.load(micIconPath))
		{
			p.drawPixmap(QRect(QPoint(m_nMarginLeft, m_nPixmapMarginTop), m_micIconSize), pix);
		}
	}
	else 
	{
		m_nBlockMarginTop = (height() - m_nBlockHeight) / 2;
	}


	int nTotal = (width() - m_nMarginLeft - m_bDrawIcon*m_micIconSize.width() - m_nPadding) / (m_nBlockWidth + m_nPadding);
	int nHits = nTotal * m_percentage;

	for (int i = 0; i < nTotal; i++)
	{
		QBrush b;
		if (i < nHits)
		{
			b = QBrush(QColor(selectedColor));
		}
		else
		{
			b = QBrush(QColor(m_unSelectedColor));
		}

		int start_x = m_nMarginLeft + m_nPadding + m_bDrawIcon*m_micIconSize.width() + i*(m_nBlockWidth + m_nPadding);
		auto rectArea = QRect(QPoint(start_x, m_nBlockMarginTop), QSize(m_nBlockWidth, m_nBlockHeight));
		p.fillRect(rectArea, b);
	}
}


