#ifndef CPROTOCOL_H
#define CPROTOCOL_H

#include "BasicTypes.h"

class CProtocol {
  public:
	CProtocol();
	virtual ~CProtocol();

	// manipulators


	// accessors

	void				ReadMessage(ISocket*, CMessage&) const;
	void				WriteMessage(ISocket*, const CMessage&) const;
};

#endif
