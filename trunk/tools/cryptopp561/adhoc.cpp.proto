#include "filters.h"
#include "files.h"
#include "base64.h"
#include "hex.h"
#include <iostream>

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

extern int (*AdhocTest)(int argc, char *argv[]);

int MyAdhocTest(int argc, char *argv[])
{
	return 0;
}

static int s_i = (AdhocTest = &MyAdhocTest, 0);
