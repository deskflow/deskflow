#ifndef ICLIENT_H
#define ICLIENT_H

class CString;

class IClient {
  public:
	IClient() { }
	virtual ~IClient() { }

	// manipulators

	// connect to server and begin processing events
	virtual void		run(const CString& hostname) = 0;
};

#endif
