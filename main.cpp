#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "CServer.h"
#include "CClient.h"
#include "CUnixTCPSocket.h"
#include "CUnixEventQueue.h"
#include "CUnixXScreen.h"

/*
static void selectMotion(Display* dpy, Window w)
{
	// select events
	XSelectInput(dpy, w, PointerMotionMask | SubstructureNotifyMask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
	if (XQueryTree(dpy, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i)
			selectMotion(dpy, cw[i]);
		XFree(cw);
	}
}

static void trackMouse(Display* dpy)
{
	// note -- this doesn't track the mouse when it's grabbed.  that's
	// okay for synergy because we don't want to cross screens then.
	selectMotion(dpy, DefaultRootWindow(dpy));
	while (true) {
		XEvent event;
		XNextEvent(dpy, &event);
		switch (event.type) {
		  case MotionNotify:
			fprintf(stderr, "mouse: %d,%d\n", event.xmotion.x_root, event.xmotion.y_root);
			break;

		  case CreateNotify:
			selectMotion(dpy, event.xcreatewindow.window);
			break;
		}
	}
}

static void checkLEDs(Display* dpy)
{
	XKeyboardState values;
	XGetKeyboardControl(dpy, &values);

	fprintf(stderr, "led (%08x): ", (unsigned int)values.led_mask);
	for (int i = 0; i < 32; ++i)
		fprintf(stderr, "%c", (values.led_mask & (1 << i)) ? 'O' : '.');
	fprintf(stderr, "\n");

	XKeyboardControl ctrl;
	for (int i = 0; i < 32; i += 2) {
		ctrl.led = i + 1;
		ctrl.led_mode = LedModeOff;
		XChangeKeyboardControl(dpy, KBLed | KBLedMode, &ctrl);
		XSync(dpy, False);
	}
}
*/

int main(int argc, char** argv)
{
/*
	printf("Hello world\n");

	Display* dpy = XOpenDisplay(NULL);

	checkLEDs(dpy);
	trackMouse(dpy);

	XCloseDisplay(dpy);
*/

	// install socket factory
	CSocketFactory::setInstance(new CUnixTCPSocketFactory);

	// create event queue
	CUnixEventQueue eventQueue;

	if (argc <= 1) {
		// create server
		CServer server;

		// create clients
		CUnixXScreen localScreen("audrey2");

		// register clients
		server.addLocalScreen(&localScreen);
		server.addRemoteScreen("remote1");

		// hook up edges
		server.connectEdge("audrey2", CServer::kLeft, "remote1");
		server.connectEdge("audrey2", CServer::kTop, "audrey2");
		server.connectEdge("audrey2", CServer::kBottom, "audrey2");
		server.connectEdge("remote1", CServer::kLeft, "audrey2");

		// do it
		server.run();
	}
	else {
		// create client
		CUnixXScreen screen("remote1");
		CClient client(&screen);
		client.run(argv[1]);
	}

	return 0;
}
