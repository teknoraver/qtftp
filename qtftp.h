#ifndef QTFTP_H
#define QTFTP_H

#include <QMainWindow>
#include <tftpd.h>

#include "ui_qtftpwidget.h"

class qtftp: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
public:
	qtftp();
private:
	Tftpd tftpd;
private slots:
	void about();
	void startServer();
	void setRoot();
};

#endif
