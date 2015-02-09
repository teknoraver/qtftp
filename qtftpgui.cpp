#include <QMessageBox>
#include <QDir>
#include <QFileDialog>

#include "qtftpgui.h"

QTftpGui::QTftpGui() : QMainWindow(0)
{
	setupUi(this);
	connect(actionAbout, SIGNAL(triggered()), SLOT(about()));
	connect(actionAbout_Qt, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(aboutQt()));

	connect(start, SIGNAL(clicked()), SLOT(startServer()));
	connect(browse, SIGNAL(clicked()), SLOT(setRoot()));
}


void QTftpGui::startServer()
{
	bool running = qtftp.isRunning();
	root->setEnabled(running);
	browse->setEnabled(running);
	if(running) {
		qtftp.terminate();
		start->setText("&Start Server");
	} else {
		qtftp.start();
		start->setText("&Stop Server");
	}
}

void QTftpGui::setRoot()
{
	QString path = QFileDialog::getExistingDirectory();
	if(path.length() && QDir(path).exists()) {
		QDir::setCurrent(path);
		root->setText(path);
	}
}

void QTftpGui::about()
{
	QMessageBox::about(this, "About QTftpGui", "QTftpGui - a Qt TFTP implementation<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
}
