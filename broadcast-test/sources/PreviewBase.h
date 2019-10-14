#ifndef VIDEOPREVIEW_H
#define VIDEOPREVIEW_H

#include <QWidget>
#include <QMenu>
#include <QImage>
#include <QTimer>
#include <mutex>
#include "BroadcastEngine/BroadcastEngine.h"

class QPushButton;

class CPreviewBase : public QWidget
{
	Q_OBJECT

public:
	CPreviewBase(QWidget *parent = 0);
	~CPreviewBase();
public:
	void udpateDesktopMask();
	void setSceneName(const QString &scene_name) { m_scene_name = scene_name; }
	void setMouseTransferEnable(bool enable);
	void resetPreview();
    void releasePreview();
protected:
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;
	virtual void resizeEvent(QResizeEvent *event) override;
	virtual QPaintEngine *paintEngine() const override;

	virtual bool isZoomMode() { return false; }
protected:
	QSharedPointer<Broadcast::SMouseEvent> qtMouseEvent2CCMouseEvent(QMouseEvent *event);

	QImage m_image;
	QTimer m_update_mask_timer;
	QString m_scene_name;

	IScenePreview *m_scene_preview = nullptr;
	bool m_mouse_transfer_enable = false;
};

#endif // VIDEOPREVIEW_H
