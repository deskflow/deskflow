#include "CServer.h"
#include "CScreenMap.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return 1;
	}

	CScreenMap screenMap;
	screenMap.addScreen("primary");
	screenMap.addScreen("ingrid");
	screenMap.connect("primary", CScreenMap::kRight, "ingrid");
	screenMap.connect("ingrid", CScreenMap::kLeft, "primary");

	try {
		CServer* server = new CServer();
		server->setScreenMap(screenMap);
		server->run();
	}
	catch (XBase& e) {
		fprintf(stderr, "failed: %s\n", e.what());
		return 1;
	}

	return 0;
}
