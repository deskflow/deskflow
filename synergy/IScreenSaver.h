#ifndef ISCREENSAVER_H
#define ISCREENSAVER_H

#include "IInterface.h"

class IScreenSaver : public IInterface {
public:
	// note -- the c'tor/d'tor must *not* enable/disable the screen saver

	// manipulators

	// enable/disable the screen saver.  enable() should restore the
	// screen saver settings to what they were when disable() was
	// previously called.  if disable() wasn't previously called then
	// it should keep the current settings or use reasonable defaults.
	virtual void		enable() = 0;
	virtual void		disable() = 0;

	// activate/deactivate (i.e. show/hide) the screen saver.
	// deactivate() also resets the screen saver timer.
	virtual void		activate() = 0;
	virtual void		deactivate() = 0;

	// accessors

	// returns true iff the screen saver is active
	virtual bool		isActive() const = 0;
};

#endif
