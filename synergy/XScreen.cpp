#include "XScreen.h"

//
// XScreenOpenFailure
//

CString
XScreenOpenFailure::getWhat() const throw()
{
	return format("XScreenOpenFailure", "unable to open screen");
}
