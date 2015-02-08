#include <QMessageBox>
#include <QDir>
#include <QFileDialog>

#include "qtftp.h"

QTftp::QTftp() : QMainWindow(0)
{
	setupUi(this);
	connect(actionAbout, SIGNAL(triggered()), SLOT(about()));
	connect(actionAbout_Qt, SIGNAL(triggered()), QCoreApplication::instance(), SLOT(aboutQt()));

	connect(start, SIGNAL(clicked()), SLOT(startServer()));
	connect(browse, SIGNAL(clicked()), SLOT(setRoot()));
}


void QTftp::startServer()
{
	bool running = tftpd.isRunning();
	root->setEnabled(running);
	browse->setEnabled(running);
	if(running) {
		tftpd.terminate();
		start->setText("&Start Server");
	} else {
		tftpd.start();
		start->setText("&Stop Server");
	}
}

void QTftp::setRoot()
{
	QString path = QFileDialog::getExistingDirectory();
	if(path.length() && QDir(path).exists()) {
		QDir::setCurrent(path);
		root->setText(path);
	}
}

void QTftp::about()
{
	QMessageBox::about(this, "About QTftp", "QTftp - a Qt TFTP implementation<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
}
