#include <QMessageBox>
#include <QDir>
#include <QFileDialog>

#include "qtftpgui.h"

QTftpGui::QTftpGui()
{
	setupUi(this);
	connect(actionAbout, &QAction::triggered, this, &QTftpGui::about);
	connect(actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

	connect(start, &QPushButton::clicked, this, &QTftpGui::startServer);
	connect(browse, &QPushButton::clicked, this, &QTftpGui::setRoot);

	connect(serverip, &QLineEdit::textChanged, this, &QTftpGui::enablePutGet);
	connect(root, &QLineEdit::textChanged, this, &QTftpGui::enableServer);

	connect(&qtftp, &QTftp::fileSent, this, &QTftpGui::sent);
	connect(&qtftp, &QTftp::fileReceived, this, &QTftpGui::received);
	connect(get, &QPushButton::clicked, this, &QTftpGui::getFile);
	connect(put, &QPushButton::clicked, this, &QTftpGui::putFile);
	connect(&qtftp, &QTftp::error, this, &QTftpGui::error);
	connect(&qtftp, &QTftp::send, progressBar, &QWidget::setEnabled);
	connect(&qtftp, &QTftp::progress, progressBar, &QProgressBar::setValue);
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
	clientgroup->setEnabled(running);
}

void QTftpGui::setRoot()
{
	QString path = QFileDialog::getExistingDirectory(this, QString(), root->text());
	if(path.length() && QDir(path).exists()) {
		QDir::setCurrent(path);
		root->setText(path);
	}
}

void QTftpGui::sent(QString file)
{
	statusbar->showMessage("sent '" + file + "'");
	progressBar->reset();
	progressBar->setEnabled(false);
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::received(QString file)
{
	statusbar->showMessage("received '" + file + "'");
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::putFile()
{
	QString path = QFileDialog::getOpenFileName();
	if(path.length() && QFile::exists(path)) {
		servergroup->setEnabled(false);
		clientgroup->setEnabled(false);
		progressBar->setEnabled(true);
		statusbar->showMessage("sending '" + path + "'");
		qtftp.put(path, serverip->text());
	}
}

void QTftpGui::getFile()
{
	QString path = QFileDialog::getSaveFileName();
	if(path.length()) {
		servergroup->setEnabled(false);
		clientgroup->setEnabled(false);
		statusbar->showMessage("receiving '" + path + "'");
		qtftp.get(path, serverip->text());
	}
}

void QTftpGui::enableServer(QString text)
{
	QDir dir(text);
	start->setEnabled(dir.exists());
}

void QTftpGui::enablePutGet(QString text)
{
	get->setEnabled(!text.isEmpty());
	put->setEnabled(!text.isEmpty());
}

void QTftpGui::error(int err)
{
	QString errorMsg;
	switch (err) {
	case QTftp::Ok:
		return;
	case QTftp::Timeout:
		errorMsg = "Operation timed out";
		break;
	case QTftp::BindError:
		errorMsg = "Failed to bind socket\nMaybe it's already bound or you need root privileges";
		break;
	case QTftp::NetworkError:
		errorMsg = "Network Error";
		break;
	default:
		errorMsg = "Error";
		break;
	}
	QMessageBox::critical(this, "Error", errorMsg);
	servergroup->setEnabled(true);
	clientgroup->setEnabled(true);
}

void QTftpGui::about()
{
	QMessageBox::about(this, "About QTftpGui", "QTftpGui - a Qt TFTP implementation<br>by Matteo Croce <a href=\"https://matteocroce.it/\">https://matteocroce.it/</a>");
}
