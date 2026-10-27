#include <QApplication>
#include "qtftpgui.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QTftpGui app;
	app.show();
	return a.exec();
}
