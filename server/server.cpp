#include "CServer.h"
#include "CScreenMap.h"
#include "CThread.h"
#include "CNetwork.h"

void					realMain()
{
	CThread::init();
	CNetwork::init();

	CScreenMap screenMap;
	screenMap.addScreen("primary");
	screenMap.addScreen("secondary");
	screenMap.connect("primary", CScreenMap::kRight, "secondary");
	screenMap.connect("secondary", CScreenMap::kLeft, "primary");

	CServer* server = NULL;
	try {
		server = new CServer();
		server->setScreenMap(screenMap);
		server->run();
		delete server;
		CNetwork::cleanup();
	}
	catch (...) {
		delete server;
		CNetwork::cleanup();
		throw;
	}
}

#if defined(CONFIG_PLATFORM_WIN32)

#include "CMSWindowsScreen.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CMSWindowsScreen::init(instance);

	if (__argc != 1) {
		CString msg = "no arguments allowed.  exiting.";
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}

	try {
		realMain();
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
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return 1;
	}

	try {
		realMain();
		return 0;
	}
	catch (XBase& e) {
		fprintf(stderr, "failed: %s\n", e.what());
		return 1;
	}
}

#endif
