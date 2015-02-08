#include <QtEndian>
#include <QFile>

#include <tftp.h>
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
		qDebug("readed %lld bytes", readed);
		struct tftphdr *tp = (struct tftphdr *)buffer;
		tp->th_opcode = qFromBigEndian(tp->th_opcode);
		qDebug("opcode: %d", tp->th_opcode);
		switch(tp->th_opcode) {
		case RRQ:
			sendfile(tp);
			break;
		default: nak(EBADOP);
		}
	}
}

void Tftpd::sendfile(struct tftphdr *tp)
{
	char *filename = tp->th_stuff;
	qDebug("serving %s", filename);
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
	while(!file.atEnd()) {
		tp->th_opcode = qToBigEndian((short)DATA);
		tp->th_block = qToBigEndian((short)block);
		readed = file.read(buffer + sizeof(tftphdr), SEGSIZE);
qDebug("sending %d", block);
		sock.writeDatagram(buffer, readed + sizeof(tftphdr), rhost, rport);
qDebug("sent");
		block++;
		tp->th_block = qToBigEndian((short)block);

		while(1) {
			QHostAddress h;
			quint16 p;
			sock.readDatagram(buffer, SEGSIZE, &h, &p);
			if(h != rhost || p != rport)
				continue;
qDebug("ACK(%d, %d)	block: %d", qFromBigEndian(tp->th_opcode), qFromBigEndian(tp->th_block), block);
			if(qFromBigEndian(tp->th_opcode) == ACK && qFromBigEndian(tp->th_block) == block - 1)
				break;
		}
qDebug("acked");
	}
	if(readed == SEGSIZE) {
		qDebug("sending 512 packet: %lu", sizeof(tftphdr));
		tp->th_opcode = qToBigEndian((short)DATA);
		tp->th_block = qToBigEndian((short)block);
		sock.writeDatagram(buffer, sizeof(tftphdr), rhost, rport);
	}
}

void Tftpd::nak(quint16 error)
{
	struct tftphdr *tp = (struct tftphdr *)buffer;
	tp->th_opcode = qToBigEndian((quint16)ERROR);
	tp->th_code = qToBigEndian(error);

	struct errmsg *pe;
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = strerror(error - 100);
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	qDebug("NAK(%d, %s)", error, pe->e_msg);
	strcpy(tp->th_msg, pe->e_msg);
	int length = strlen(pe->e_msg);
	tp->th_msg[length] = 0;
	length += 5;
	sock.writeDatagram(buffer, length, rhost, rport);
}
