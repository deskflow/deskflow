#include "CTCPSocket.h"
#include "CTCPListenSocket.h"
#include "CNetworkAddress.h"
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CThread.h"
#include "CFunctionJob.h"
#include <stdio.h>

//
// test
//

SInt16 port = 50000;

static void				thread1(void*)
{
   try {
	fprintf(stdout, "client started\n");
	CThread::sleep(1.0);
	CNetworkAddress addr("127.0.0.1", port);

	fprintf(stdout, "client connecting\n");
	CTCPSocket* socket = new CTCPSocket;
	socket->connect(addr);

	fprintf(stdout, "client connected (%p).  waiting.\n", socket);
	CThread::sleep(2.0);

	fprintf(stdout, "client sending message\n");
	static const char msg[] = "message from client\n";
	socket->getOutputStream()->write(msg, sizeof(msg) - 1);
	socket->getOutputStream()->flush();

	fprintf(stdout, "client waiting for reply\n");
	UInt8 buffer[4096];
	UInt32 n;
	do {
		n = socket->getInputStream()->read(buffer, sizeof(buffer) - 1);
		buffer[n] = 0;
		fprintf(stdout, "%s", buffer);
	} while (n == 0 && memchr(buffer, '\n', n) == NULL);

	fprintf(stdout, "client closing\n");
	socket->close();
	delete socket;
	fprintf(stdout, "client terminating\n");
  }
  catch (XBase& e) {
	fprintf(stderr, "exception: %s\n", e.what());
  }
}

static void				thread2(void*)
{
  try {
	fprintf(stdout, "server started\n");
	CNetworkAddress addr("127.0.0.1", port);
	CTCPListenSocket listenSocket;
	listenSocket.bind(addr);

	fprintf(stdout, "server accepting\n");
	ISocket* socket = listenSocket.accept();
	fprintf(stdout, "server accepted %p\n", socket);

	UInt8 buffer[4096];
	UInt32 n;
	do {
		n = socket->getInputStream()->read(buffer, sizeof(buffer) - 1);
		buffer[n] = 0;
		fprintf(stdout, "%s", buffer);
	} while (n == 0 && memchr(buffer, '\n', n) == NULL);

	fprintf(stdout, "server replying\n");
	static const char reply[] = "data received\n";
	socket->getOutputStream()->write(reply, sizeof(reply) - 1);

	fprintf(stdout, "server closing\n");
	socket->close();
	delete socket;
	fprintf(stdout, "server terminating\n");
  }
  catch (XBase& e) {
	fprintf(stderr, "exception: %s\n", e.what());
  }
}

static void				thread3(void*)
{
   try {
	fprintf(stdout, "### looking up address\n");
	CNetworkAddress addr("www.google.com", 80);

	fprintf(stdout, "### connecting\n");
	CTCPSocket* socket = new CTCPSocket;
	socket->connect(addr);

	fprintf(stdout, "### sending message\n");
	static const char msg[] = "GET / HTTP/1.0\nAccept: */*\n\n";
	socket->getOutputStream()->write(msg, sizeof(msg) - 1);
	socket->getOutputStream()->flush();
	socket->getOutputStream()->close();

	fprintf(stdout, "### waiting for reply\n");
	UInt8 buffer[4096];
	UInt32 n;
	do {
		n = socket->getInputStream()->read(buffer, sizeof(buffer) - 1);
		buffer[n] = 0;
		fprintf(stdout, "%s", buffer);
	} while (n != 0);

	fprintf(stdout, "### closing\n");
	socket->close();
	delete socket;
	fprintf(stdout, "### terminating\n");
  }
  catch (XBase& e) {
	fprintf(stderr, "### exception: %s\n", e.what());
  }
}

int main(int argc, char** argv)
{
/*
	if (argc > 1) {
		port = (SInt16)atoi(argv[1]);
	}

	fprintf(stdout, "starting threads\n");
	CThread t1(new CFunctionJob(thread1));
	CThread t2(new CFunctionJob(thread2));

	fprintf(stdout, "waiting for threads\n");
	t1.wait();
	t2.wait();
	fprintf(stdout, "threads finished\n");
*/
	int tick = 0;
	CThread t3(new CFunctionJob(thread3));
	while (!t3.wait(0.1)) {
		fprintf(stdout, "$$$ %d\n", ++tick);
	}
	return 0;
}
