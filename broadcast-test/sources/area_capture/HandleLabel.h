#ifndef HANDLELABEL_H
#define HANDLELABEL_H

#include <QLabel>

class CHandleLabel : public QLabel
{
	Q_OBJECT

public:
	CHandleLabel(QWidget *parent);
	~CHandleLabel();
protected:
	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void mouseMoveEvent(QMouseEvent *ev);
	void paintEvent(QPaintEvent *event);
private:
	QPoint m_pointPress;
	bool m_bPress;
};

#endif // HANDLELABEL_H
