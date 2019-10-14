#include "OBSSettingWindow.h"
#include <QtWidgets/QApplication>
#include <QDir>
#include <QDebug>

#include "BroadcastEngine/BroadcastEngine.h"
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
#ifdef WIN32
	QDir::setCurrent(a.applicationDirPath());
#endif
	auto pEngine = CBroadcastEngine::instance()->engine();
	auto bRet = pEngine->init();
	if (bRet == Broadcast::INITIAL_OK)
	{
		qDebug() << "init error";
	}

	OBSSettingWindow w;
	w.show();

	return a.exec();
}
