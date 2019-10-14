#ifndef AREACAPTUREFRAME_H
#define AREACAPTUREFRAME_H

#include <QtWidgets/QWidget>
#include "ui_AreaCaptureFrame.h"
#include <QTimer>
#include <functional>
#include <memory>


class IAreaCaptureSourceProperty;

class CAreaCaptureFrame : public QWidget
{
	Q_OBJECT

	enum StretchPoint
	{
		Stretch_None,
		Stretch_TopLeft,
		Stretch_TopRight,
		Stretch_BottomLeft,
		Stretch_BottomRight,
	};

public:
	CAreaCaptureFrame(QWidget *parent = 0);
	~CAreaCaptureFrame();
public:
	void init(std::shared_ptr<IAreaCaptureSourceProperty>);
private:
	void setAreaRect(const QRect &rect);
	QRect getAreaRect();
	void setStretchEnabled(bool enable);
	void registerWheelReceiver(QObject *receiver){ m_pWheelReceiver = receiver; }
	inline void setSourceName(const QString &sourceName) { m_sSourceName = sourceName; }
	void setStretchVisible(bool visible);
	void updateAreaFrameGeometry(const QRect &rect,const std::function<bool (int left,int right)> &comareFun = std::less_equal<int>());
protected:
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
private slots:
	void slotUpdateAreaGeometry();
	void slotAdjustAreaByProportion();
	void adjustAreaByProportion(const QRect & rect, const std::function<bool(int left, int right)> &comareFun = std::less_equal<int>());
private:
	void checkMouseMove(QMouseEvent *e);
	StretchPoint selectStetchPoint(QMouseEvent *e);
private:
	Ui::CAreaCaptureFrameClass ui;
	QPoint m_pointPress;
	bool m_bMousePress;
	QRect m_rect;
	StretchPoint m_eStretchPoint;
	bool m_bStetchEnabled;
	bool m_bStetchVisible = true;

	QString m_sSourceName;
	QTimer m_timer;

	QObject *m_pWheelReceiver;

	std::shared_ptr<IAreaCaptureSourceProperty> m_areaCaptureProperty;
};

#endif // AREACAPTUREFRAME_H
