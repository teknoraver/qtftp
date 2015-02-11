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

	connect(&qtftp, SIGNAL(fileSent(QString)), SLOT(sent(QString)));
	connect(&qtftp, SIGNAL(fileReceived(QString)), SLOT(received(QString)));
	connect(get, SIGNAL(clicked()), SLOT(getFile()));
	connect(put, SIGNAL(clicked()), SLOT(putFile()));
}


void QTftpGui::startServer()
{
	bool running = qtftp.isRunning();
	root->setEnabled(running);
	browse->setEnabled(running);
	if(running) {
		qtftp.stopServer();
		start->setText("&Start Server");
		statusbar->showMessage("Server stopped");
	} else {
		qtftp.startServer();
		start->setText("&Stop Server");
		statusbar->showMessage("Server started");
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

void QTftpGui::sent(QString file)
{
	statusbar->showMessage("sent '" + file + "'");
}

void QTftpGui::received(QString file)
{
	statusbar->showMessage("received '" + file + "'");
}

void QTftpGui::putFile()
{
	QString path = QFileDialog::getOpenFileName();
	if(path.length() && QFile::exists(path)) {
		statusbar->showMessage("sending '" + path + "'");
		qtftp.put(path, "127.0.0.1");
	}
}

void QTftpGui::getFile()
{
	QString path = QFileDialog::getSaveFileName();
	if(path.length()) {
		statusbar->showMessage("receiving '" + path + "'");
		qtftp.get(path, "127.0.0.1");
	}
}

void QTftpGui::about()
{
	QMessageBox::about(this, "About QTftpGui", "QTftpGui - a Qt TFTP implementation<br>by Matteo Croce <a href=\"http://teknoraver.net/\">http://teknoraver.net/</a>");
}
