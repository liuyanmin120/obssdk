#include "AreaCaptureRect.h"
#include <QPainter>
#include <QMouseEvent>

CAreaCaptureRect::CAreaCaptureRect(QWidget *parent)
	: QWidget(parent)
	, m_nSpace(5),
	m_stectchVisible(true)
{
	setMouseTracking(true);
}

CAreaCaptureRect::~CAreaCaptureRect()
{

}

void CAreaCaptureRect::setStectchVisible(bool bVisible)
{
	m_stectchVisible = bVisible;
}

void CAreaCaptureRect::paintEvent(QPaintEvent *event)
{
	// 反走样
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setBrush(QColor(0, 0, 0, 0));
	QPen pen;
	pen.setColor(QColor(0, 255, 0, 200));
	pen.setWidth(m_nSpace);
	painter.setPen(pen);
	QPoint topLeft(m_nSpace / 2, m_nSpace / 2);
	m_nSpace = 2;
	//painter.drawRect(topLeft.x(), topLeft.y(), width() - m_nSpace, height() - m_nSpace);
	painter.drawLine(QPoint(topLeft.x(), topLeft.y()), QPoint(width() - m_nSpace, topLeft.y()));
	painter.drawLine(QPoint(width() - m_nSpace, topLeft.y()), QPoint(width() - m_nSpace, height() - m_nSpace));
	painter.drawLine(QPoint(width() - m_nSpace, height() - m_nSpace), QPoint(topLeft.x(), height() - m_nSpace));
	painter.drawLine(QPoint(topLeft.x(), height() - m_nSpace), QPoint(topLeft.x(), topLeft.y()));

	if (m_stectchVisible)
	{
		pen.setColor(QColor(255, 0, 0, 255));
		painter.setPen(pen);
	}
	m_nSpace = 5;
	int lineLength = 15;
	painter.drawLine(QPoint(topLeft.x(), topLeft.y() + lineLength), topLeft);
	painter.drawLine(QPoint(topLeft.x(), topLeft.y()), QPoint(topLeft.x() + lineLength, topLeft.y()));
	painter.drawLine(QPoint(topLeft.x() + width() - m_nSpace - lineLength, topLeft.y()), QPoint(topLeft.x() + width() - m_nSpace + lineLength, topLeft.y()));
	painter.drawLine(QPoint(topLeft.x() + width() - m_nSpace, topLeft.y()), QPoint(topLeft.x() + width() - m_nSpace, topLeft.y() + lineLength));
	painter.drawLine(QPoint(topLeft.x() + width() - m_nSpace, topLeft.y() + height() - m_nSpace - lineLength), QPoint(topLeft.x() + width() - m_nSpace, topLeft.y() + height()));
	painter.drawLine(QPoint(topLeft.x() + width() - m_nSpace - lineLength, topLeft.y() + height() - m_nSpace), QPoint(topLeft.x() + width() - m_nSpace, topLeft.y() + height() - m_nSpace));
	painter.drawLine(QPoint(topLeft.x(), topLeft.y() + height() - m_nSpace - lineLength), QPoint(topLeft.x(), topLeft.y() + height()));
	painter.drawLine(QPoint(topLeft.x(), topLeft.y() + height() - m_nSpace), QPoint(topLeft.x() + lineLength, topLeft.y() + height() - m_nSpace));

}

void CAreaCaptureRect::mouseMoveEvent(QMouseEvent *e)
{
	e->ignore();
	return;
}

bool CAreaCaptureRect::eventFilter(QObject * watched, QEvent * event)
{
	return QWidget::eventFilter(watched, event);
}
