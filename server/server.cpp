#include "CServer.h"
#include "CConfig.h"
#include "CLog.h"
#include "CMutex.h"
#include "CNetwork.h"
#include "CThread.h"
#include "XThread.h"
#include "stdfstream.h"
#include <assert.h>

//
// config file stuff
//

static const char* s_configFileName = "synergy.conf";


//
// logging thread safety
//

static CMutex*			s_logMutex = NULL;

static void				logLock(bool lock)
{
	assert(s_logMutex != NULL);

	if (lock) {
		s_logMutex->lock();
	}
	else {
		s_logMutex->unlock();
	}
}


//
// main
//

void					realMain()
{
	// initialize threading library
	CThread::init();

	// make logging thread safe
	CMutex logMutex;
	s_logMutex = &logMutex;
	CLog::setLock(&logLock);

	CServer* server = NULL;
	try {
		// initialize network library
		CNetwork::init();

		// load configuration
		CConfig config;
		{
			log((CLOG_DEBUG "opening configuration"));
			std::ifstream configStream(s_configFileName);
			if (!configStream) {
				throw XConfigRead("cannot open configuration");
			}
			configStream >> config;
			log((CLOG_DEBUG "configuration read successfully"));
		}

		// run server
		server = new CServer();
		server->setConfig(config);
		server->run();

		// clean up
		delete server;
		CNetwork::cleanup();
		CLog::setLock(NULL);
		s_logMutex = NULL;
	}
	catch (...) {
		delete server;
		CNetwork::cleanup();
		CLog::setLock(NULL);
		s_logMutex = NULL;
		throw;
	}
}


//
// platform dependent entry points
//

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
		log((CLOG_CRIT "failed: %s", e.what()));
		CString msg = "failed: ";
		msg += e.what();
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}
	catch (XThread&) {
		// terminated
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
		log((CLOG_CRIT "failed: %s", e.what()));
		fprintf(stderr, "failed: %s\n", e.what());
		return 1;
	}
	catch (XThread&) {
		// terminated
		return 1;
	}
}

#endif
