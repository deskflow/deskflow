#include "CClient.h"
#include "CString.h"
#include "CNetwork.h"
#include "CNetworkAddress.h"
#include "CThread.h"

void					realMain(const CString& name,
								const CString& hostname,
								UInt16 port)
{
	CThread::init();
	CNetwork::init();

	CClient* client = NULL;
	try {
		CNetworkAddress addr(hostname, port);
		client = new CClient(name);
		client->run(addr);
		delete client;
		CNetwork::cleanup();
	}
	catch (...) {
		delete client;
		CNetwork::cleanup();
		throw;
	}
}

#if defined(CONFIG_PLATFORM_WIN32)

#include "CMSWindowsScreen.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CMSWindowsScreen::init(instance);

	if (__argc != 2) {
		CString msg = "hostname required.  exiting.";
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}

	try {
		realMain("secondary", __argv[1], 50001);
		return 0;
	}
	catch (XBase& e) {
		CString msg = "failed: ";
		msg += e.what();
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}
}

#else

#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <hostname>\n", argv[0]);
		return 1;
	}

	try {
		realMain("secondary", argv[1], 50001);
		return 0;
	}
	catch (XBase& e) {
		fprintf(stderr, "failed: %s\n", e.what());
		return 1;
	}
}

#endif
