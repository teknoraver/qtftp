#ifndef TFTPD_H
#define TFTPD_H

#include <QUdpSocket>
#include <QThread>

#define SEGSIZE 512

class Tftpd : public QThread
{
Q_OBJECT

private:
	QUdpSocket *sock;
	void run();
	QHostAddress rhost;
	quint16 rport;

	enum Block : quint16 {
		RRQ	= 1,	// read request
		WRQ	= 2,	// write request
		DATA	= 3,	// data packet
		ACK	= 4,	// acknowledgement
		ERROR	= 5	// error code
	};

	enum Error : quint16 {
		EUNDEF		= 0,	/* not defined */
		ENOTFOUND	= 1,	/* file not found */
		EACCESS		= 2,	/* access violation */
		ENOSPACE	= 3,	/* disk full or allocation exceeded */
		EBADOP		= 4,	/* illegal TFTP operation */
		EBADID		= 5,	/* unknown transfer ID */
		EEXISTS		= 6,	/* file already exists */
		ENOUSER		= 7	/* no such user */
	};

	struct errmsg {
		int e_code;
		const char *e_msg;
	} errmsgs[9] = {
		{ EUNDEF,	"Undefined error code" },
		{ ENOTFOUND,	"File not found" },
		{ EACCESS,	"Access violation" },
		{ ENOSPACE,	"Disk full or allocation exceeded" },
		{ EBADOP,	"Illegal TFTP operation" },
		{ EBADID,	"Unknown transfer ID" },
		{ EEXISTS,	"File already exists" },
		{ ENOUSER,	"No such user" },
		{ -1,			0 }
	};

	struct tftp_header {
		quint16 opcode;
		union {
			struct {
				quint16 block;
				char data[0];
			} data;
			char path[0];
		};
	};

	char buffer[SEGSIZE + sizeof(tftp_header)];

	void sendfile(struct tftp_header*);
	void getfile(struct tftp_header*);
	void nak(Error error);
	void ack(quint16 block);
};

#endif
