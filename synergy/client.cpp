#include "CClient.h"
#include "CNetworkAddress.h"
#include <stdio.h>

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <hostname>\n", argv[0]);
		return 1;
	}

	try {
		CClient* client = new CClient("ingrid");
		client->run(CNetworkAddress(argv[1], 50001));
	}
	catch (XBase& e) {
		fprintf(stderr, "failed: %s\n", e.what());
		return 1;
	}
	return 0;
}
