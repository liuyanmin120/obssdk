#include "PreviewBase.h"
#include <QMouseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWindow>
#include <QPushButton>
#include <QPainter>
#include "BroadcastEngine/BroadcastEngine.h"

//#include "module/eos/EosObserver.h"


#ifdef WIN32
#include <dwmapi.h>
#endif


CPreviewBase::CPreviewBase(QWidget *parent)
	: QWidget(parent)
{
 	setUpdatesEnabled(false);

	connect(&m_update_mask_timer, &QTimer::timeout, [this]()
	{
		udpateDesktopMask();
	});
	m_update_mask_timer.start(100);

	connect(CBroadcastEngine::instance(), &CBroadcastEngine::signalOutputSizeChange
		, this, [this](int width, int height)
	{
		(void)width;
		(void)height;
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (isVisible()) {
            resetPreview();
        }
	},Qt::QueuedConnection);

	m_scene_name = DEFAULT_SCENE;
}

CPreviewBase::~CPreviewBase()
{
    m_update_mask_timer.stop();
	if (CBroadcastEngine::engine())
	{
		auto scene_manager = CBroadcastEngine::engine()->sceneManager();
		scene_manager->releasePreview(m_scene_preview);
	}

}


void CPreviewBase::mousePressEvent(QMouseEvent *event)
{
	if (m_scene_preview)
	{
		QSharedPointer<Broadcast::SMouseEvent> ev = qtMouseEvent2CCMouseEvent(event);
		const char * srcName = m_scene_preview->mousePressEvent(ev.data());

		int srcType = Broadcast::NONE_SOURCE;
		CBroadcastEngine::engine()->enumSource([&](Broadcast::ESourceType type, const char *src_name)
		{
			if (srcName && strcmp(srcName, src_name) == 0)
			{
				srcType = type;
				return false;
			}
			return true;
		});
		if (event->button() == Qt::RightButton && srcName
			/*&& srcType != CC::MEDIA_TALK_SOURCE*/)
		{
#ifdef WIN32
			QMenu m_popup;

			m_popup.addAction(QObject::tr("delete"), [&]
			{
				//GEosObserver::notify("deleteSource", QVariantList() << QString::fromUtf8(srcName));
			});	//删除

			m_popup.addAction(QObject::tr("rename"), [&]
			{

				//GEosObserver::notify("enterToRenameMode", QVariantList() << QString::fromUtf8(srcName));
			});	//重命名

			m_popup.addAction(QObject::tr("Up"), [&]
			{
				//CBroadcastEngine::engine()->manipulateSource(srcName, CC::CCSourceActiton::ORDER_MOVE_UP);
				//GEosObserver::notify("notifyUpdateSource", QVariant());
			});//向上移动
			m_popup.addAction(QObject::tr("Down"), [&]
			{
				//CBroadcastEngine::engine()->manipulateSource(srcName, CC::CCSourceActiton::ORDER_MOVE_DOWN);
				//GEosObserver::notify("notifyUpdateSource", QVariant());

			});//向下移动
			m_popup.addAction(QObject::tr("top"), [&]
			{
				//CBroadcastEngine::engine()->manipulateSource(srcName, CC::CCSourceActiton::ORDER_MOVE_TOP);
				//GEosObserver::notify("notifyUpdateSource", QVariant());

			});//移至顶部
			m_popup.addAction(QObject::tr("Bottom"), [&]
			{
				//CBroadcastEngine::engine()->manipulateSource(srcName, CC::CCSourceActiton::ORDER_MOVE_BOTTOM);
				//GEosObserver::notify("notifyUpdateSource", QVariant());

			});//移至底部

			//拉升至全屏
			m_popup.addAction(QObject::tr("Original proportional stretch"), [&]			//原比例拉伸
			{
				//CBroadcastEngine::engine()->manipulateSource(srcName, CC::CCSourceActiton::FIT_TO_SCREEN);
				//GEosObserver::notify("notifyUpdateSource", QVariant());
			});

			m_popup.addAction(QObject::tr("Properties"), [&]
			{
				//Broadcast::ESourceType type = CBroadcastEngine::engine()->getSourceIDByName(srcName);
				//GEosObserver::notify("showPropertiesWindow", QVariantList() << (int)type << QString::fromUtf8(srcName));
			});
			m_popup.exec(QCursor::pos());
#endif // WIN32
		}
	}
	QWidget::mousePressEvent(event);
}

void CPreviewBase::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_scene_preview)
	{
		QSharedPointer<Broadcast::SMouseEvent> ev = qtMouseEvent2CCMouseEvent(event);
		m_scene_preview->mouseReleaseEvent(ev.data());
	}
	QWidget::mouseReleaseEvent(event);
}

#include <QDebug>
void CPreviewBase::mouseMoveEvent(QMouseEvent *event)
{
	if (m_scene_preview)
	{
		QSharedPointer<Broadcast::SMouseEvent> ev = qtMouseEvent2CCMouseEvent(event);
		m_scene_preview->mouseMoveEvent(ev.data());
	}
	QWidget::mouseMoveEvent(event);
}

void CPreviewBase::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
}

void CPreviewBase::hideEvent(QHideEvent * event)
{
	QWidget::hideEvent(event);
}

void CPreviewBase::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	//CGlobalData::instance()->setPreviewSize(width(), height());
    qDebug() << "preview width = " << width() <<",height= "<< height();

    static std::once_flag m_callOnceFlag;
	std::call_once(m_callOnceFlag, [this]
	{
		/*if (CGlobalData::instance()->getPublicModule())
		{
			QTimer::singleShot(600, this, [this]
			{
				GEosObserver::notify("showVideoDevice");
			});

		}*/

	});
	udpateDesktopMask();

	resetPreview();
}

QPaintEngine * CPreviewBase::paintEngine() const
{
	return Q_NULLPTR;
}

QSharedPointer<Broadcast::SMouseEvent> CPreviewBase::qtMouseEvent2CCMouseEvent(QMouseEvent *event)
{
	QSharedPointer<Broadcast::SMouseEvent> ev(new Broadcast::SMouseEvent);
	ev->x = event->pos().x();
	ev->y = event->pos().y();
	ev->button = event->button() == Qt::LeftButton ?Broadcast::EMouseButton::MOUSE_LEFT :Broadcast::EMouseButton::MOUSE_RIGHT;
	switch (event->modifiers())
	{
	case Qt::ShiftModifier:
		ev->modifiers = Broadcast::KEY_SHIFT;
		break;
	case Qt::ControlModifier:
		ev->modifiers = Broadcast::KEY_CONTROL;
	case Qt::AltModifier:
		ev->modifiers = Broadcast::KEY_ALT;
	default:
		ev->modifiers = Broadcast::KEY_NONE;
		break;
	}
	return ev;
}

void CPreviewBase::udpateDesktopMask()
{
//	if (!CBroadcastEngine::engine() || CGlobalData::instance()->getPublicModule() == 1)
//		return;
//
//	auto point = mapToGlobal(pos());
//	auto mask_data = CBroadcastEngine::engine()->getMonitorMask();
//	if (!mask_data)
//	{
//		mask_data = std::make_shared<CC::MaskData>();
//	}
//
//#ifdef WIN32
//	BOOL aero_enable = TRUE;
//	auto ret = DwmIsCompositionEnabled(&aero_enable);
//	mask_data->enable = aero_enable == TRUE;
//#endif
//
//	if (mask_data->x != point.x()
//		|| mask_data->y != point.y()
//		|| mask_data->width != width()
//		|| mask_data->height != height())
//	{
//		m_image = QImage(width(), height(), QImage::Format_RGB32);
//		m_image.fill(Qt::black);
//		QPainter painter(&m_image);
//		QFont f;
//		f.setPixelSize(100);
//		painter.setFont(f);
//		painter.setPen(Qt::red);
//		QFontMetrics fm(f);
//
//		auto text_width = fm.width(tr("Desktop"));
//		auto text_height = fm.height();
//
//		QRect target_rect((width() - text_width) / 2
//			, (height() - text_height) / 2
//			, width()
//			, height());
//		painter.drawText(target_rect,tr("Desktop"));
//
//		mask_data->x = point.x()/*-5*/;
//		mask_data->y = point.y()/*-7*/;
//		mask_data->width = width();
//		mask_data->height = height();
//		mask_data->size = m_image.byteCount();
//		mask_data->data = m_image.bits();
//		CBroadcastEngine::engine()->updateMonitorMask(mask_data);
//	}
}

void CPreviewBase::setMouseTransferEnable(bool enable)
{
	if (m_scene_preview)
	{
		m_scene_preview->setMouseTransferEnable(enable);
	}
	m_mouse_transfer_enable = enable;
}
void CPreviewBase::releasePreview()
{
	//engine_log(CCLOG_INFO, "release scene preview.zoom mode:%s", isZoomMode() ? "true" : "false");
	if (isZoomMode())
	{
		return;
	}
    CBroadcastEngine::engine()->sceneManager()->releasePreview(m_scene_preview);
    m_scene_preview = nullptr;
}
void CPreviewBase::resetPreview()
{
	//engine_log(CCLOG_INFO, "reset scene preview.zoom mode:%s", isZoomMode() ? "true" : "false");
	if (isZoomMode())
	{
		return;
	}
	auto scene_manager = CBroadcastEngine::engine()->sceneManager();
	scene_manager->releasePreview(m_scene_preview);
	m_scene_preview = scene_manager->createPreview((void*)winId());
	m_scene_preview->bindScene(m_scene_name.toUtf8().data());
	m_scene_preview->setMouseTransferEnable(m_mouse_transfer_enable);
	m_scene_preview->update();
}
