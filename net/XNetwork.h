#ifndef XNETWORK_H
#define XNETWORK_H

#include "XBase.h"
#include "CString.h"

class XNetwork : public XBase { };

class XNetworkUnavailable : public XNetwork {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XNetworkFailed : public XNetwork {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XNetworkVersion : public XNetwork {
public:
	XNetworkVersion(int major, int minor) throw();

	// accessors

	int					getMajor() const throw();
	int					getMinor() const throw();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	int					m_major;
	int					m_minor;
};

class XNetworkFunctionUnavailable : public XNetwork {
public:
	XNetworkFunctionUnavailable(const char* name) throw();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

#endif
