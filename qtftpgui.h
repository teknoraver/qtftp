#ifndef QTFTPGUI_H
#define QTFTPGUI_H

#include <QMainWindow>
#include <qtftp.h>

#include "ui_qtftpguiwidget.h"

class QTftpGui: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
public:
	QTftpGui();
private:
	QTftp qtftp;
private slots:
	void about();
	void startServer();
	void setRoot();
	void sent(QString);
	void received(QString);
	void getFile();
	void putFile();
};

#endif
