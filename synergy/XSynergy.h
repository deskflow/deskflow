#ifndef XSYNERGY_H
#define XSYNERGY_H

#include "XBase.h"

class XSynergy : public XBase { };

// client is misbehaving
class XBadClient : public XSynergy {
  protected:
	virtual CString		getWhat() const throw();
};

// client has incompatible version
class XIncompatibleClient : public XSynergy {
  public:
	XIncompatibleClient(int major, int minor);

	// manipulators

	// accessors

	int					getMajor() const throw();
	int					getMinor() const throw();

  protected:
	virtual CString		getWhat() const throw();

  private:
	int					m_major;
	int					m_minor;
};

#endif
