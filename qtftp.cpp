#include <QtEndian>
#include <QFile>

#include <qtftp.h>

void QTftp::run()
{
	sock = new QUdpSocket(this);
	connect(this, SIGNAL(finished()), sock, SLOT(deleteLater()));
	sock->bind(PORT);
	while(1) {
		sock->waitForReadyRead(-1);
		qint64 readed = sock->readDatagram(buffer, SEGSIZE, &rhost, &rport);
		if(readed < 0)
			continue;

		struct tftp_header *th = (struct tftp_header *)buffer;
		th->opcode = qFromBigEndian(th->opcode);
		switch(th->opcode) {
		case RRQ:
			server_put();
			break;
		case WRQ:
			server_get();
			break;
		default: nak(EBADOP);
		}
	}
	sock->close();
	delete sock;
}

void QTftp::server_put()
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	qDebug("sending %s", th->path);
	if(QString(th->path).contains('/')) {
		nak(EACCESS);
		return;
	}
	QFile file(th->path);
	if(!file.open(QIODevice::ReadOnly))
		switch (file.error()) {
		case QFile::OpenError:
			nak(ENOTFOUND);
			return;
		case QFile::PermissionsError:
			nak(EACCESS);
			return;
		default:
			nak(EUNDEF);
			return;
		}

	quint64 readed;
	quint16 block = 1;
	do {
		th->opcode = qToBigEndian((quint16)DATA);
		th->data.block = qToBigEndian((quint16)block);
		readed = file.read(buffer + sizeof(struct tftp_header), SEGSIZE);

		sock->writeDatagram(buffer, readed + sizeof(struct tftp_header), rhost, rport);

		waitForAck(block++);
	} while(readed == SEGSIZE);
	qDebug("sent %d blocks, %llu bytes", (block - 1), (block - 2) * SEGSIZE + readed);
}

void QTftp::server_get()
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	char *filename = th->path;
	qDebug("receiving %s", filename);
	QFile file(filename);
	if(!file.open(QIODevice::WriteOnly))
		switch (file.error()) {
		case QFile::PermissionsError:
			nak(EACCESS);
			return;
		default:
			nak(EUNDEF);
			return;
		}

	sendAck(0);
	quint64 received;
	quint16 block = 1;
	do {
		while(1) {
			QHostAddress h;
			quint16 p;
			sock->waitForReadyRead(-1);
			received = sock->readDatagram(buffer, SEGSIZE + sizeof(struct tftp_header), &h, &p);

			if(h != rhost || p != rport)
				continue;

			if(qFromBigEndian(th->opcode) == DATA && qFromBigEndian(th->data.block) == block)
				break;
		}
		file.write(buffer + sizeof(struct tftp_header), received - sizeof(struct tftp_header));
		sendAck(block++);
	} while (received == SEGSIZE + sizeof(struct tftp_header));
	qDebug("received %d blocks, %llu bytes", block - 1, (block - 2) * SEGSIZE + received);
}

void QTftp::client_get(QString path, QString server)
{
	sock = new QUdpSocket(this);
	connect(this, SIGNAL(finished()), sock, SLOT(deleteLater()));

	struct tftp_header *th = (struct tftp_header *)buffer;
	strcpy(th->path, path.toUtf8().constData());
	strcpy(th->path + path.length() + 1, "octect");
	qDebug("receiving %s", th->path);
	QFile file(th->path);
	if(!file.open(QIODevice::WriteOnly))
		return;

	th->opcode = qToBigEndian((quint16)RRQ);

	sock->bind();
	sock->writeDatagram(buffer, sizeof(struct tftp_header) + path.length() + sizeof("octect") - 1, QHostAddress(server), PORT);

	qint64 readed;
	quint16 block = 1;
	do {
		sock->waitForReadyRead(-1);
		readed = sock->readDatagram(buffer, SEGSIZE + (sizeof(struct tftp_header)), &rhost, &rport);

		file.write(th->data.data, readed - sizeof(struct tftp_header));
		sendAck(block++);
	} while (readed == SEGSIZE + sizeof(tftp_header));
}

void QTftp::waitForAck(quint16 block)
{
	while(1) {
		struct tftp_header th;
		QHostAddress h;
		quint16 p;
		sock->waitForReadyRead();
		sock->readDatagram((char *)&th, sizeof(struct tftp_header), &h, &p);

		if(h != rhost || p != rport)
			continue;

		if(qFromBigEndian(th.opcode) == ACK && qFromBigEndian(th.data.block) == block)
			break;
	}
}

void QTftp::sendAck(quint16 block)
{
	struct tftp_header ack;
	ack.opcode = qToBigEndian((quint16)ACK);
	ack.data.block = qToBigEndian(block);
	sock->writeDatagram((char*)&ack, sizeof(struct tftp_header), rhost, rport);
}

void QTftp::nak(Error error)
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	th->opcode = qToBigEndian((quint16)ERROR);
	th->data.block = qToBigEndian((quint16)error);

	struct errmsg *pe;
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		th->data.block = EUNDEF;   /* set 'undef' errorcode */
	}

	strcpy(th->data.data, pe->e_msg);
	int length = strlen(pe->e_msg);
	th->data.data[length] = 0;
	length += 5;
	sock->writeDatagram(buffer, length, rhost, rport);
}
