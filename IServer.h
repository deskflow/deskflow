#ifndef ISERVER_H
#define ISERVER_H

class IScreen;

class IServer {
  public:
	IServer() { }
	virtual ~IServer() { }

	// manipulators

	// run the server until terminated
	virtual void		run() = 0;

	// clipboard operations
	virtual void		onClipboardChanged(IScreen*) = 0;

	// enter the given screen, leaving the previous screen.  the cursor
	// should be warped to the center of the screen.
	virtual void		setActiveScreen(IScreen*) = 0;

	// accessors

	// get the screen that was last entered
	virtual IScreen*	getActiveScreen() const = 0;
};

#endif
