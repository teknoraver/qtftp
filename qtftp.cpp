#include <QtEndian>
#include <QFile>
#include <QFileInfo>

#include <qtftp.h>

void QTftp::server()
{
	qDebug("Starting server");
	sock = new QUdpSocket(this);
	connect(&worker, SIGNAL(finished()), sock, SLOT(deleteLater()));
	connect(this, SIGNAL(error(int)), sock, SLOT(deleteLater()));
	if(!sock->bind(PORT)) {
		sock->close();
		emit error(BindError);
		return;
	}
	while(true) {
		sock->waitForReadyRead(-1);
		qint64 readed = sock->readDatagram(buffer, SEGSIZE + sizeof(struct tftp_header), &rhost, &rport);
		if(readed < 0)
			continue;

		struct tftp_header *th = (struct tftp_header *)buffer;
		switch(qFromBigEndian(th->opcode)) {
		case RRQ:
			server_get();
			break;
		case WRQ:
			server_put();
			break;
		default: nak(EBADOP);
		}
	}
	sock->close();
	delete sock;
}

void QTftp::startServer()
{
	moveToThread(&worker);
	connect(this, SIGNAL(doServer()), this, SLOT(server()));
	worker.start();
	emit doServer();
}

void QTftp::stopServer()
{
	worker.terminate();
}

void QTftp::server_get()
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	QString path = th->path;
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

	emit send(true);
	quint64 readed;
	quint16 block = 1;
	do {
		qint64 blocks = file.size() / 512;
		int percent = -1;

		th->opcode = qToBigEndian((quint16)DATA);
		th->data.block = qToBigEndian((quint16)block);
		readed = file.read(buffer + sizeof(struct tftp_header), SEGSIZE);

		int i;
		for(i = 0; i < RETRIES; i++) {
			sock->writeDatagram(buffer, readed + sizeof(struct tftp_header), rhost, rport);
			if(waitForAck(block++))
				break;
		}
		if(i == RETRIES) {
			emit error(Timeout);
			return;
		}
		int newp = block * 100 / blocks;
		if(newp > percent) {
			percent = newp;
			emit progress(newp);
		}
	} while(readed == SEGSIZE);
	emit fileSent(path);
	qDebug("sent %d blocks, %llu bytes", (block - 1), (block - 2) * SEGSIZE + readed);
}

void QTftp::server_put()
{
	struct tftp_header *th = (struct tftp_header *)buffer;
	QString path = th->path;
	qDebug("receiving %s", th->path);
	QFile file(path);
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
		while(true) {
			QHostAddress h;
			quint16 p;
			if(!sock->waitForReadyRead(TIMEOUT)) {
				emit error(Timeout);
				return;
			}
			received = sock->readDatagram(buffer, SEGSIZE + sizeof(struct tftp_header), &h, &p);

			if(h != rhost || p != rport)
				continue;

			if(th->opcode == qToBigEndian((quint16)DATA) && qFromBigEndian(th->data.block) == block)
				break;
		}
		file.write(buffer + sizeof(struct tftp_header), received - sizeof(struct tftp_header));
		sendAck(block++);
	} while (received == SEGSIZE + sizeof(struct tftp_header));
	emit fileReceived(path);
	qDebug("received %d blocks, %llu bytes", block - 1, (block - 2) * SEGSIZE + received);
}

void QTftp::client_get(QString path, QString server)
{
	qDebug("receiving %s", path.toUtf8().constData());
	QFile file(path);
	if(!file.open(QIODevice::WriteOnly))
		return;

	QFileInfo name(path);
	struct tftp_header *th = (struct tftp_header *)buffer;
	strcpy(th->path, name.fileName().toUtf8().constData());
	strcpy(th->path + name.fileName().length() + 1, "octect");

	sock = new QUdpSocket(this);
	sock->bind();
	connect(&worker, SIGNAL(finished()), sock, SLOT(deleteLater()));
	connect(this, SIGNAL(error(int)), sock, SLOT(deleteLater()));

	th->opcode = qToBigEndian((quint16)RRQ);

	int i;
	for(i = 0; i < RETRIES; i++) {
		if(sock->writeDatagram(buffer, sizeof(struct tftp_header) + name.fileName().length() + sizeof("octect") - 1, QHostAddress(server), PORT) <= 0) {
			emit error(NetworkError);
			return;
		}
		if(waitForAck(0))
			break;
	}
	if(i == RETRIES) {
		emit error(Timeout);
		return;
	}

	qint64 readed;
	quint16 block = 1;
	do {
		if(!sock->waitForReadyRead(TIMEOUT)) {
			emit error(Timeout);
			return;
		}
		readed = sock->readDatagram(buffer, SEGSIZE + (sizeof(struct tftp_header)), &rhost, &rport);

		file.write(th->data.data, readed - sizeof(struct tftp_header));
		sendAck(block++);
	} while (readed == SEGSIZE + sizeof(tftp_header));
	sock->close();
	delete sock;
	emit fileReceived(name.fileName());
}

void QTftp::client_put(QString path, QString server)
{
	qDebug("sending %s", path.toUtf8().constData());
	QFile file(path);
	if(!file.open(QIODevice::ReadOnly))
		return;

	QFileInfo name(path);
	struct tftp_header *th = (struct tftp_header *)buffer;
	strcpy(th->path, name.fileName().toUtf8().constData());
	strcpy(th->path + name.fileName().length() + 1, "octect");

	sock = new QUdpSocket(this);
	sock->bind();
	connect(&worker, SIGNAL(finished()), sock, SLOT(deleteLater()));
	connect(this, SIGNAL(error(int)), sock, SLOT(deleteLater()));

	th->opcode = qToBigEndian((quint16)WRQ);

	int i;
	for(i = 0; i < RETRIES; i++) {
		if(sock->writeDatagram(buffer, sizeof(struct tftp_header) + name.fileName().length() + sizeof("octect") - 1, QHostAddress(server), PORT) <= 0) {
			emit error(NetworkError);
			return;
		}
		rhost.clear();
		rport = 0;
		if(waitForAck(0))
			break;
	}
	if(i == RETRIES) {
		emit error(Timeout);
		return;
	}

	quint64 readed;
	quint16 block = 1;
	do {
		qint64 blocks = file.size() / 512;
		int percent = -1;

		th->opcode = qToBigEndian((quint16)DATA);
		th->data.block = qToBigEndian((quint16)block);
		readed = file.read(buffer + sizeof(struct tftp_header), SEGSIZE);

		for(i = 0; i < RETRIES; i++) {
			if(sock->writeDatagram(buffer, readed + sizeof(struct tftp_header), rhost, rport) > 0) {
				emit error(NetworkError);
				return;
			}
			if(waitForAck(block++))
				break;
		}
		if(i == RETRIES) {
			emit error(Timeout);
			return;
		}
		int newp = block * 100 / blocks;
		if(newp > percent) {
			percent = newp;
			emit progress(newp);
		}
	} while(readed == SEGSIZE);
	qDebug("sent %d blocks, %llu bytes", (block - 1), (block - 2) * SEGSIZE + readed);
	sock->close();
	delete sock;
	emit fileSent(name.fileName());
}

void QTftp::get(QString path, QString server)
{
	moveToThread(&worker);
	connect(this, SIGNAL(doGet(QString, QString)), this, SLOT(client_get(QString, QString)));
	worker.start();
	emit doGet(path, server);
}

void QTftp::put(QString path, QString server)
{
	moveToThread(&worker);
	connect(this, SIGNAL(doPut(QString, QString)), this, SLOT(client_put(QString, QString)));
	worker.start();
	emit doPut(path, server);
}

bool QTftp::isRunning()
{
	return worker.isRunning();
}

bool QTftp::waitForAck(quint16 block)
{
	for(int i = 0; i < RETRIES; i++) {
		struct tftp_header th;
		QHostAddress h;
		quint16 p;

		if(!sock->waitForReadyRead(TIMEOUT))
			continue;

		sock->readDatagram((char *)&th, sizeof(struct tftp_header), &h, &p);

		if(rhost.isNull() && rport == 0) {
			rhost = h;
			rport = p;
		} else if(h != rhost || p != rport)
			continue;

		if(th.opcode == qToBigEndian((quint16)ACK) && qFromBigEndian(th.data.block) == block)
			return true;
	}
	emit error(Timeout);
}

void QTftp::sendAck(quint16 block)
{
	struct tftp_header ack;
	ack.opcode = qToBigEndian((quint16)ACK);
	ack.data.block = qToBigEndian(block);
	sock->writeDatagram((char*)&ack, sizeof(struct tftp_header), rhost, rport);
}

void QTftp::nak(TftpError error)
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
