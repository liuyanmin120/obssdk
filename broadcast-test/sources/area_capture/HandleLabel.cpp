#include "HandleLabel.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDesktopWidget>

CHandleLabel::CHandleLabel(QWidget *parent)
	: QLabel(parent)
	, m_bPress(false)
{
}

CHandleLabel::~CHandleLabel()
{

}

void CHandleLabel::mousePressEvent(QMouseEvent *ev)
{
	QLabel::mousePressEvent(ev);
	m_bPress = true;
	m_pointPress = ev->pos();
}

void CHandleLabel::mouseReleaseEvent(QMouseEvent *ev)
{
	QLabel::mouseReleaseEvent(ev);
	m_bPress = false;
}

void CHandleLabel::mouseMoveEvent(QMouseEvent *ev)
{
	QLabel::mouseMoveEvent(ev);
	if (m_bPress)
	{
		if (QWidget *parent = parentWidget())
		{
			QPoint nowParPoint = parent->pos();
			nowParPoint.setX(nowParPoint.x() + ev->x() - m_pointPress.x());
			nowParPoint.setY(nowParPoint.y() + ev->y() - m_pointPress.y());

			QDesktopWidget desk;
			if (nowParPoint.x() < 0)
				nowParPoint.setX(0);
			if (nowParPoint.y() < 0)
				nowParPoint.setY(0);
			if (nowParPoint.x() + parent->width() > desk.width())
				nowParPoint.setX(desk.width() - parent->width());
			if (nowParPoint.y() + parent->height() > desk.height())
				nowParPoint.setY(desk.height() - parent->height());

			parent->move(nowParPoint);
		}
	}
}

void CHandleLabel::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setBrush(QBrush(QColor(0, 255, 0, 230)));
	painter.setPen(Qt::NoPen);
	painter.drawRect(0, 0, width(), height());
	QLabel::paintEvent(event);
}
