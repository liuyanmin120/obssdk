#ifndef REDRECT_H
#define REDRECT_H

#include <QWidget>

class CAreaCaptureRect : public QWidget
{
	Q_OBJECT
public:
	CAreaCaptureRect(QWidget *parent);
	~CAreaCaptureRect();
public:
	inline void setSpace(int space){ m_nSpace = space; }
	inline qint32 getSpace(){ return m_nSpace; }

	void setStectchVisible(bool bVisible);

protected:
	void paintEvent(QPaintEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	bool eventFilter(QObject *watched, QEvent *event);
private:
	qint32 m_nSpace;
	bool m_stectchVisible;
};

#endif // REDRECT_H
