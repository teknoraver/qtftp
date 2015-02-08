#include <QtEndian>
#include <QFile>

#include <tftpd.h>

Tftpd::Tftpd()
{
	start();
}

void Tftpd::run()
{
	sock.bind(69);
	while(1) {
		sock.waitForReadyRead(-1);
		qint64 readed = sock.readDatagram(buffer, SEGSIZE, &rhost, &rport);
		if(readed < 0)
			continue;

		struct tfth_header *tp = (struct tfth_header *)buffer;
		tp->opcode = qFromBigEndian(tp->opcode);
		switch(tp->opcode) {
		case RRQ:
			sendfile(tp);
			break;
		default: nak(EBADOP);
		}
	}
}

void Tftpd::sendfile(struct tfth_header *tp)
{
	char *filename = tp->path;
	qDebug("sending %s", filename);
	QFile file(filename);
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
		tp->opcode = qToBigEndian((quint16)DATA);
		tp->data.block = qToBigEndian((quint16)block);
		readed = file.read(buffer + sizeof(struct tfth_header), SEGSIZE);

		sock.writeDatagram(buffer, readed + sizeof(struct tfth_header), rhost, rport);

		block++;
		tp->data.block = qToBigEndian((quint16)block);

		while(1) {
			QHostAddress h;
			quint16 p;
			sock.readDatagram(buffer, SEGSIZE, &h, &p);
			if(h != rhost || p != rport)
				continue;

			if(qFromBigEndian(tp->opcode) == ACK && qFromBigEndian(tp->data.block) == block - 1)
				break;
		}
	} while(readed == SEGSIZE);
	qDebug("sent %d blocks, %d bytes", (block - 1), (block - 2) * SEGSIZE + readed);
}

void Tftpd::nak(quint16 error)
{
	struct tfth_header *tp = (struct tfth_header *)buffer;
	tp->opcode = qToBigEndian((quint16)ERROR);
	tp->data.block = qToBigEndian(error);

	struct errmsg *pe;
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->data.block = EUNDEF;   /* set 'undef' errorcode */
	}

	strcpy(tp->data.data, pe->e_msg);
	int length = strlen(pe->e_msg);
	tp->data.data[length] = 0;
	length += 5;
	sock.writeDatagram(buffer, length, rhost, rport);
}
