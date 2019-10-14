#include "AreaCaptureFrame.h"
#include <QMouseEvent>
#include "BroadcastEngine/BroadcastEngine.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QApplication>
#include "EosObserver.h"

CAreaCaptureFrame::CAreaCaptureFrame(QWidget *parent)
	: QWidget(parent)
	, m_bMousePress(false)
	, m_pWheelReceiver(nullptr)
	, m_bStetchEnabled(true)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint  | Qt::Tool);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setMouseTracking(true);

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotUpdateAreaGeometry()));
	GEosObserver::attach("adjustAreaCaptureByProportion", this, SLOT(slotAdjustAreaByProportion()));
}

CAreaCaptureFrame::~CAreaCaptureFrame()
{
	slotUpdateAreaGeometry();
}

void CAreaCaptureFrame::slotAdjustAreaByProportion()
{


	updateAreaFrameGeometry(this->geometry(),std::greater_equal<int>());
}

void CAreaCaptureFrame::adjustAreaByProportion(const QRect & wndRect, const std::function<bool(int left, int right)> &comareFun)
{
	int nSpace = ui.widget_rect->getSpace();
	int labelHeight = ui.label->height();

	int w = 0, h = 0;

	CBroadcastEngine::engine()->getOutputSize(w, h);
	float proportion = h*1.0f / w;
	if (w == 0 || h == 0)
	{
		setGeometry(wndRect);
		return;
	}

	int nMinWidth = 320;
	int nMinHeight = proportion * 320;

	QRect areaCaptureRect;
	areaCaptureRect.setLeft(wndRect.left() + nSpace);
	areaCaptureRect.setTop(wndRect.top() + nSpace + labelHeight);
	areaCaptureRect.setBottom(wndRect.bottom() - nSpace);
	areaCaptureRect.setRight(wndRect.right() - nSpace);


	int areaWidth = areaCaptureRect.height() / proportion;
	if (comareFun(areaWidth, areaCaptureRect.width()))
	{
		int areaHeight = areaCaptureRect.width()*proportion;
		if (areaHeight <= nMinHeight)
		{
			areaCaptureRect.setWidth(nMinWidth);
			areaCaptureRect.setHeight(nMinHeight);

		}
		else
		{
			areaCaptureRect.setHeight(areaHeight);
		}

	}
	else
	{
		if (areaWidth <= nMinWidth)
		{
			areaCaptureRect.setWidth(nMinWidth);
			areaCaptureRect.setHeight(nMinHeight);
		}
		else
		{
			areaCaptureRect.setWidth(areaWidth);
		}

	}

	QRect afterAdjustRect;

	afterAdjustRect.setTop(areaCaptureRect.top() - labelHeight - nSpace);
	afterAdjustRect.setLeft(areaCaptureRect.left() - nSpace);
	afterAdjustRect.setRight(areaCaptureRect.right() + nSpace);
	afterAdjustRect.setBottom(areaCaptureRect.bottom() + nSpace);

	this->setMinimumWidth(nMinWidth + nSpace * 2);
	this->setMinimumHeight(nMinHeight + nSpace * 2 + labelHeight);
	
	setGeometry(afterAdjustRect);

}
	

void CAreaCaptureFrame::checkMouseMove(QMouseEvent *e)
{
	int nSpace = 15;
	QRect curRect = m_rect;
	if (m_bMousePress && m_eStretchPoint != Stretch_None)
	{
		int offsetX = QCursor::pos().x() - m_pointPress.x();
		int offsetY = QCursor::pos().y() - m_pointPress.y();

		QRect topLeftBoundRect = m_rect;
		topLeftBoundRect.setLeft(m_rect.left() + m_rect.size().width() - minimumSize().width());
		topLeftBoundRect.setTop(m_rect.top() + m_rect.size().height() - minimumSize().height());

		QRect topRightBoundRect = m_rect;
		topRightBoundRect.setRight(m_rect.left() - m_rect.size().width() + minimumSize().width());
		topRightBoundRect.setTop(m_rect.top() + m_rect.size().height() - minimumSize().height());

		QRect bottomLeftBoundRect = m_rect;
		bottomLeftBoundRect.setLeft(m_rect.left() + m_rect.size().width() - minimumSize().width());
		bottomLeftBoundRect.setTop(m_rect.top() - m_rect.size().height() + minimumSize().height());

		switch (m_eStretchPoint)
		{
		case CAreaCaptureFrame::Stretch_TopLeft:
			curRect.setTopLeft(QPoint(m_rect.left() + offsetX, m_rect.top() + offsetY));
			if (curRect.left() > topLeftBoundRect.left())
			{
				curRect.setLeft(topLeftBoundRect.left());
			}
			if (curRect.top() > topLeftBoundRect.top())
			{
				curRect.setTop(topLeftBoundRect.top());
			}
			break;
		case CAreaCaptureFrame::Stretch_TopRight:
			curRect.setTopRight(QPoint(m_rect.right() + offsetX, m_rect.top() + offsetY));
			if (curRect.right() < topRightBoundRect.right())
			{
				curRect.setRight(topRightBoundRect.right());
			}
			if (curRect.top() > topRightBoundRect.top())
			{
				curRect.setTop(topRightBoundRect.top());
			}
			break;
		case CAreaCaptureFrame::Stretch_BottomLeft:
			curRect.setBottomLeft(QPoint(m_rect.left() + offsetX, m_rect.bottom() + offsetY));
			if (curRect.left() > bottomLeftBoundRect.left())
			{
				curRect.setLeft(bottomLeftBoundRect.left());
			}
			if (curRect.top() < bottomLeftBoundRect.top())
			{
				curRect.setTop(bottomLeftBoundRect.top());
			}
			break;
		case CAreaCaptureFrame::Stretch_BottomRight:
			curRect.setBottomRight(QPoint(m_rect.right() + offsetX, m_rect.bottom() + offsetY));
			break;
		default:
			break;
		}

		updateAreaFrameGeometry(curRect);
	}
	else
	{
		QRect RBrect(width() - nSpace, height() - nSpace, nSpace, nSpace);
		QRect LTrect(0, ui.label->height(), nSpace, nSpace);
		QRect RTrect(width() - nSpace, ui.label->height(), nSpace, nSpace);
		QRect LBrect(0, height() - nSpace, nSpace, nSpace);
		if (RBrect.contains(e->pos()) || LTrect.contains(e->pos()))
			setCursor(Qt::SizeFDiagCursor);
		else if (LBrect.contains(e->pos()) || RTrect.contains(e->pos()))
			setCursor(Qt::SizeBDiagCursor);
		else
			setCursor(Qt::ArrowCursor);
	}
}

void CAreaCaptureFrame::mouseMoveEvent(QMouseEvent *e)
{
	if (m_bStetchEnabled)
	{
		checkMouseMove(e);
	}
	QWidget::mouseMoveEvent(e);
}

void CAreaCaptureFrame::mousePressEvent(QMouseEvent *e)
{
	QWidget::mousePressEvent(e);

	if (e->button() == Qt::LeftButton)
	{
		m_pointPress = QCursor::pos();
		m_bMousePress = true;
		m_rect = geometry();
		m_eStretchPoint = selectStetchPoint(e);
	}
}

void CAreaCaptureFrame::mouseReleaseEvent(QMouseEvent *e)
{
	QWidget::mouseReleaseEvent(e);
	m_bMousePress = false;
	setCursor(Qt::ArrowCursor);
}

void CAreaCaptureFrame::init(std::shared_ptr<IAreaCaptureSourceProperty> srcProperty)
{
	m_areaCaptureProperty = srcProperty;
	Broadcast::SRect rect; 
	m_areaCaptureProperty->getAreaCaptureParam(rect);
	setAreaRect(QRect(rect.x,rect.y,rect.width,rect.height));
}

void CAreaCaptureFrame::setAreaRect(const QRect &rect)
{
	QRect myRect = rect;
	qint32 space = ui.widget_rect->getSpace();
	int labelHeight = ui.label->height();
	myRect.setTop(rect.top() - labelHeight - space);
	myRect.setLeft(rect.left() - space);
	myRect.setRight(rect.right() + space);
	myRect.setBottom(rect.bottom() + space);
	
	updateAreaFrameGeometry(myRect);
}

QRect CAreaCaptureFrame::getAreaRect()
{
	QRect myRect = geometry();
	qint32 space = ui.widget_rect->getSpace();
	int labelHeight = ui.label->height();
	myRect.setTop(myRect.top() + labelHeight + space);
	myRect.setLeft(myRect.left() + space);
	myRect.setRight(myRect.right() - space);
	myRect.setBottom(myRect.bottom() - space);
	return myRect;
}

void CAreaCaptureFrame::moveEvent(QMoveEvent *event)
{
	QWidget::moveEvent(event);

	QPoint p(event->pos());
	if (event->pos().x() < 0)
	{
		p.setX(0);
	}
	if (event->pos().y() < 0)
	{
		p.setY(0);
	}
	if (p != event->pos())
	{
		move(p);
	}
	if (m_timer.isActive())
	{
		m_timer.stop();
    }
    m_timer.start(40);
}

void CAreaCaptureFrame::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	if (m_timer.isActive())
	{
		m_timer.stop();
    }
    m_timer.start(40);
}

void CAreaCaptureFrame::slotUpdateAreaGeometry()
{
	m_timer.stop();
	Broadcast::SRect param;
	QRect rect = getAreaRect();
#ifdef WIN32
    auto ratio = 1;
#else
    auto ratio = devicePixelRatio();
#endif
	param.x = rect.x() * ratio;
	param.y = rect.y() * ratio;
	param.width = rect.width() * ratio;
	param.height = rect.height() * ratio;

	m_areaCaptureProperty->setAreaCaptureParam(param);

	QString strGeometry = QString("%1,%2,%3,%4")
		.arg(geometry().x())
		.arg(geometry().y())
		.arg(geometry().width())
		.arg(geometry().height());

	///config_set_bool(m_pEngine->Config(), "SimpleOutput", "hasRect", true);
	//config_set_string(m_pEngine->Config(), "SimpleOutput", "rect", strGeometry.toUtf8().data());

	QDesktopWidget* desktopWidget = QApplication::desktop();
	//获取可用桌面大小
	QRect deskRect = desktopWidget->availableGeometry();
	//获取设备屏幕大小
	QRect screenRect = desktopWidget->screenGeometry();

	strGeometry = QString("%1,%2")
		.arg(screenRect.width())
		.arg(screenRect.height());

	//config_set_string(m_pEngine->Config(), "SimpleOutput", "desktop_rect", strGeometry.toUtf8().data());

	ui.label->setText(QString("%1(%2x%3)")
		.arg(QObject::tr("Click here to drag"))
		.arg(param.width/ratio)
		.arg(param.height/ratio));
    
    raise();
}

void CAreaCaptureFrame::setStretchEnabled(bool enable)
{
	m_bStetchEnabled = enable;
}

void CAreaCaptureFrame::setStretchVisible(bool visible)
{
	ui.widget_rect->setStectchVisible(false);
}

void CAreaCaptureFrame::updateAreaFrameGeometry(const QRect & rect, const std::function<bool(int left, int right)> &comareFun)
{
	//if (!CGlobalData::instance()->getPublicModule())
	//{
	//	bool isNeedAdjust = CSettings::instance()->value("AreaCapture/adjustProportion", true).toBool();
	//	if (isNeedAdjust) 
	//	{
	//		adjustAreaByProportion(rect, comareFun);
	//		return;
	//	}

	//}
	adjustAreaByProportion(rect, comareFun);
	setGeometry(rect);
}

CAreaCaptureFrame::StretchPoint CAreaCaptureFrame::selectStetchPoint(QMouseEvent *e)
{
	int nSpace = 15;
	QRect RBrect(width() - nSpace, height() - nSpace, nSpace, nSpace);
	QRect LTrect(0, ui.label->height(), nSpace, nSpace);
	QRect RTrect(width() - nSpace, ui.label->height(), nSpace, nSpace);
	QRect LBrect(0, height() - nSpace, nSpace, nSpace);

	if (RBrect.contains(e->pos()))
	{
		return Stretch_BottomRight;
	}
	else if (LTrect.contains(e->pos()))
	{
		return Stretch_TopLeft;
	}
	else if (LBrect.contains(e->pos()))
	{
		return Stretch_BottomLeft;
	}
	else if (RTrect.contains(e->pos()))
	{
		return Stretch_TopRight;
	}
	return Stretch_None;
}
