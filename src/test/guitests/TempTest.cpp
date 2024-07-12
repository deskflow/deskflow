#include "TempTest.h"

TempTest::TempTest(StreamProvider *sp) {
  auto stream = sp->makeStream();
  stream->writeRawData("test", 4);
}