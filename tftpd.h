#ifndef TFTPD_H
#define TFTPD_H

#include <QUdpSocket>
#include <QThread>

#include <tftp.h>

class Tftpd : public QThread
{
Q_OBJECT

public:
	Tftpd();

private:
	QUdpSocket sock;
	void run();
	char buffer[SEGSIZE + sizeof(tftphdr)];
	QHostAddress rhost;
	quint16 rport;

	void sendfile(struct tftphdr*);
	void nak(quint16 error);
};

static struct errmsg {
	int e_code;
	const char *e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

#endif
