#pragma once
#include <QWidget>
#include <QTimer>

class MicrophoneVolumeCtrl :
	public QWidget
{
	Q_OBJECT
public:
	MicrophoneVolumeCtrl(QWidget *);
	virtual ~MicrophoneVolumeCtrl();
	void init(int nBlockWidth,int nBlockHeight,bool drawIcon = false);
	void setSrcName(const QString &srcName) 
	{
		m_srcName = srcName; 
		resetSignals();
	}
	void resetSignals();
protected:
	void	paintEvent(QPaintEvent *event)override;

private:
	float   m_percentage = 0.0f;
	int m_nBlockWidth = 6;
	int m_nBlockHeight = 16;
	int m_nPadding = 6;
	int m_nMarginLeft = 6;
	int m_nPixmapMarginTop = 2;
	int m_nBlockMarginTop = 4;
	QSize m_micIconSize = QSize(20, 20);
	bool  m_bDrawIcon = true;
	QString  m_unSelectedColor;
	QMetaObject::Connection m_curConnect;
	QString  m_srcName;
	bool  m_srcIsActive = false;
};

