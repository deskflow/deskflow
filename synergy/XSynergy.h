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

// client has duplicate name (i.e. client with name is already connected)
class XDuplicateClient : public XSynergy {
public:
	XDuplicateClient(const CString& name);

	// manipulators

	// accessors

	virtual const CString&
						getName() const throw();

protected:
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

// client has unknown name (i.e. name is not in server's screen map)
class XUnknownClient : public XSynergy {
public:
	XUnknownClient(const CString& name);

	// manipulators

	// accessors

	virtual const CString&
						getName() const throw();

protected:
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

#endif
