#ifndef CSOCKETFACTORY_H
#define CSOCKETFACTORY_H

#define CSOCKETFACTORY CSocketFactory::getInstance()

class ISocket;

class CSocketFactory {
  public:
	CSocketFactory();
	virtual ~CSocketFactory();

	// manipulators

	static void			setInstance(CSocketFactory*);

	// accessors

	// create a socket
	virtual ISocket*	create() const = 0;

	// get the global instance
	static CSocketFactory*	getInstance();

  private:
	static CSocketFactory*	s_instance;
};

#endif
