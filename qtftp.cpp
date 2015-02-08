#include <QMessageBox>

#include "qtftp.h"

qtftp::qtftp() : QMainWindow(0)
{
	setupUi(this);
	connect(actionAbout, SIGNAL(triggered()), SLOT(about()));
	connect(actionAbout_Qt, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(aboutQt()));
}

qtftp::~qtftp()
{
}

void qtftp::about()
{
        QMessageBox::about(this, "About qtftp", "qtftp - a Qt qtftp application<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
}
