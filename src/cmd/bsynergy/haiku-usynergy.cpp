/*
 * Copyright (c)
 *      2014  Ed Robbins <edd.robbins@gmail.com>
 *      2014  Jessica Hamilton <jessica.l.hamilton@gmail.com>
 *		2015  Alexander von Gluck IV <kallisti5@unixzen.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <Notification.h>
#include <OS.h>
#include <Path.h>
#include <PathFinder.h>
#include <PathMonitor.h>
#include <Screen.h>
#include <TranslationUtils.h>

#include <cstdlib>
#include <strings.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <driver_settings.h>
#include <keyboard_mouse_driver.h>

#include "ATKeymap.h"


#include "haiku-usynergy.h"


#define TRACE_SYNERGY_DEVICE
#ifdef TRACE_SYNERGY_DEVICE
#	define TRACE(x...) \
		do { debug_printf(x); } while (0)
#else
#	define TRACE(x...) do {} while (0)
#endif

#define FILE_UPDATED 'fiUp'


const static uint32 kSynergyThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;


// Static hook functions for uSynergy

static uSynergyBool
uConnect(uSynergyCookie cookie)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	return device->Connect();
}


static uSynergyBool
uSend(uSynergyCookie cookie, const uint8* buffer, int length)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	return device->Send(buffer, length);
}


static uSynergyBool
uReceive(uSynergyCookie cookie, uint8* buffer, int maxLength, int* outLength) {
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	if ((uSynergyInputServerDevice*)device->Receive(buffer, maxLength, outLength))
		return USYNERGY_TRUE;
	return USYNERGY_FALSE;
}


static void
uSleep(uSynergyCookie cookie, int milliseconds)
{
	snooze(milliseconds * 1000);
}


static uint32_t
uGetTime()
{
	return system_time() / 1000;
}


static void
uTrace(uSynergyCookie cookie, const char* text) {
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->Trace(text);
}


static void
uScreenActive(uSynergyCookie cookie, uSynergyBool active) {
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->ScreenActive(active);
}


static void
uMouseCallback(uSynergyCookie cookie, uint16 x, uint16 y,
	int16 wheelX, int16 wheelY, uSynergyBool buttonLeft,
	uSynergyBool buttonRight, uSynergyBool buttonMiddle)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->MouseCallback(x, y, wheelX, wheelY, buttonLeft,
		buttonRight, buttonMiddle);
}


static void
uKeyboardCallback(uSynergyCookie cookie, uint16 key, uint16 modifiers,
	uSynergyBool isKeyDown, uSynergyBool isKeyRepeat)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->KeyboardCallback(key, modifiers, isKeyDown, isKeyRepeat);
}


static void
uJoystickCallback(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons,
	int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->JoystickCallback(joyNum, buttons, leftStickX, leftStickY,
		rightStickX, rightStickY);
}


static void
uClipboardCallback(uSynergyCookie cookie, enum uSynergyClipboardFormat format,
	const uint8_t* data, uint32_t size)
{
	uSynergyInputServerDevice* device = (uSynergyInputServerDevice*)cookie;
	device->ClipboardCallback(format, data, size);
}


uSynergyInputServerDevice::uSynergyInputServerDevice()
	:
	BHandler("uSynergy Handler"),
	threadActive(false),
	fContext(NULL),
	fSocket(-1),
	fEnableSynergy(false),
	fServerAddress(NULL),
	fServerKeymap(NULL),
	fUpdateSettings(false),
	fKeymapLock("synergy keymap lock")
{
	fContext = (uSynergyContext*)malloc(sizeof(uSynergyContext));
	uSynergyInit(fContext);

	fContext->m_connectFunc				= uConnect;
	fContext->m_receiveFunc				= uReceive;
	fContext->m_sendFunc				= uSend;
	fContext->m_getTimeFunc				= uGetTime;
	fContext->m_screenActiveCallback	= uScreenActive;
	fContext->m_mouseCallback			= uMouseCallback;
	fContext->m_keyboardCallback		= uKeyboardCallback;
	fContext->m_sleepFunc				= uSleep;
	fContext->m_traceFunc				= uTrace;
	fContext->m_joystickCallback		= uJoystickCallback;
	fContext->m_clipboardCallback		= uClipboardCallback;
	fContext->m_clientName				= "haiku";
	fContext->m_cookie					= (uSynergyCookie)this;

	BRect screenRect = BScreen().Frame();
	fContext->m_clientWidth		= (uint16_t)screenRect.Width() + 1;
	fContext->m_clientHeight	= (uint16_t)screenRect.Height() + 1;

	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
		path.Append("synergy_settings");

	fFilename = new char[strlen(path.Path()) + 1];
	strcpy(fFilename, path.Path());

	BEntry entry(fFilename);

	BPrivate::BPathMonitor::StartWatching(fFilename, B_WATCH_STAT | B_WATCH_FILES_ONLY, this);

	_UpdateSettings();
}


uSynergyInputServerDevice::~uSynergyInputServerDevice()
{
	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}

	free(fContext);
}


status_t
uSynergyInputServerDevice::InitCheck()
{
	input_device_ref *devices[3];

	input_device_ref mouse = { (char*)"uSynergy Mouse", B_POINTING_DEVICE, (void *)this };
	input_device_ref keyboard = { (char*)"uSynergy Keyboard", B_KEYBOARD_DEVICE, (void *)this };

	devices[0] = &mouse;
	devices[1] = &keyboard;
	devices[2] = NULL;

	RegisterDevices(devices);

	return B_OK;
}


void
uSynergyInputServerDevice::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PATH_MONITOR:
		{
			const char* path = "";
			// only fall through for appropriate file
			if (!(message->FindString("path", &path) == B_OK
					&& strcmp(path, fFilename) == 0)) {
				break; // not the file we're looking for
			}
		}
		// fall-through
		case FILE_UPDATED:
		{
			_UpdateSettings();
			if (threadActive)
				Stop(NULL, NULL);
			Start(NULL, NULL);
			break;
		}
		case B_CLIPBOARD_CHANGED:
		{
			const char *text = NULL;
			ssize_t len = 0;
			BMessage *clip = NULL;
			if (be_clipboard->Lock()) {
				if ((clip = be_clipboard->Data()) == B_OK) {
					clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &len);
				}
				be_clipboard->Unlock();
			}
			if (len > 0 && text != NULL) {
				uSynergySendClipboard(fContext, text);
				TRACE("synergy: data added to clipboard\n");
			} else
				TRACE("synergy: couldn't add data to clipboard\n");
		}
		default:
			BHandler::MessageReceived(message);
	}
}


status_t
uSynergyInputServerDevice::Start(const char* name, void* cookie)
{
	if (fServerAddress.Length() == 0 || fEnableSynergy == false) {
		TRACE("synergy: not enabled, or no server specified\n");
		return B_NO_ERROR;
	}

	status_t status = B_OK;
	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "uSynergy haiku");

	TRACE("synergy: thread active = %d\n", threadActive);

	if ((atomic_get_and_set((int32*)&threadActive, true) & true) == true) {
		TRACE("synergy: main thread already running\n");
		return B_OK;
	}

	uSynergyThread = spawn_thread(_MainLoop, threadName, kSynergyThreadPriority, (void*)this);

	if (uSynergyThread < 0) {
		threadActive = false;
		status = uSynergyThread;
		TRACE("synergy: spawn thread failed: %" B_PRIx32 "\n", status);
	} else {
		be_clipboard->StartWatching(this);
		status = resume_thread(uSynergyThread);
	}

	return status;
}


status_t
uSynergyInputServerDevice::Stop(const char* name, void* cookie)
{
	threadActive = false;
	// this will stop the thread as soon as it reads the next packet
	be_clipboard->StopWatching(this);

	if (uSynergyThread >= 0) {
		// unblock the thread, which might wait on a semaphore.
		suspend_thread(uSynergyThread);
		resume_thread(uSynergyThread);
		status_t dummy;
		wait_for_thread(uSynergyThread, &dummy);
	}

	return B_OK;
}


status_t
uSynergyInputServerDevice::SystemShuttingDown()
{
	threadActive = false;

	return B_OK;
}


status_t
uSynergyInputServerDevice::Control(const char* name, void* cookie, uint32 command, BMessage* message)
{
	if (command == B_KEY_MAP_CHANGED) {
		fUpdateSettings = true;
		return B_OK;
	}

	return B_BAD_VALUE;
}


status_t
uSynergyInputServerDevice::_MainLoop(void* arg)
{
	uSynergyInputServerDevice *inputDevice = (uSynergyInputServerDevice*)arg;

	while (inputDevice->threadActive) {
		uSynergyUpdate(inputDevice->fContext);

		if (inputDevice->fUpdateSettings) {
			inputDevice->_UpdateSettings();
			inputDevice->fUpdateSettings = false;
		}
	}

	close(inputDevice->fSocket);
	inputDevice->fSocket = -1;

	return B_OK;
}


void
uSynergyInputServerDevice::_UpdateSettings()
{
	BAutolock lock(fKeymapLock);
	fKeymap.RetrieveCurrent();
	fModifiers = fKeymap.Map().lock_settings;
	fControlKey = fKeymap.KeyForModifier(B_LEFT_CONTROL_KEY);
	fCommandKey = fKeymap.KeyForModifier(B_LEFT_COMMAND_KEY);

	void* handle = load_driver_settings(fFilename);
	if (handle == NULL)
		return;

	fEnableSynergy = get_driver_boolean_parameter(handle, "enable", false, false);
	fServerKeymap = get_driver_parameter(handle, "server_keymap", NULL, NULL);
	fServerAddress = get_driver_parameter(handle, "server", NULL, NULL);

	unload_driver_settings(handle);
}


bool
uSynergyInputServerDevice::Connect()
{
	if (fServerAddress.Length() == 0 || fEnableSynergy == false)
		goto exit;

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(24800);
	inet_aton(fServerAddress.String(), &server.sin_addr);

	TRACE("synergy: connecting to %s:%d\n", fServerAddress.String(), 24800);

	fSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (fSocket < 0) {
		TRACE("synergy: socket couldn't be created\n");
		goto exit;
	}

	if (connect(fSocket, (struct sockaddr*)&server, sizeof(struct sockaddr)) < 0 ) {
		TRACE("synergy: %s: %x\n", "failed to connect to remote host", errno);
		close(fSocket);
		fSocket = -1;
		goto exit;
	}
	else
		return true;
exit:
	snooze(1000000);
	return false;
}


bool
uSynergyInputServerDevice::Send(const uint8_t* buffer, int32_t length)
{
	if (send(fSocket, buffer, length, 0) != length)
		return false;
	return true;
}


bool
uSynergyInputServerDevice::Receive(uint8_t *buffer, int maxLength, int* outLength)
{
	if ((*outLength = recv(fSocket, buffer, maxLength, 0)) == -1)
		return false;
	return true;
}


void
uSynergyInputServerDevice::Trace(const char *text)
{
	BNotification notify(B_INFORMATION_NOTIFICATION);
	BString group("Synergy");
	BString content(text);

	notify.SetGroup(group);
	notify.SetContent(content);
	BBitmap* bitmap = BTranslationUtils::GetBitmap("/boot/home/config/non-packaged/data/synergy-32.png");
	if (bitmap != NULL)
		notify.SetIcon(bitmap);
	notify.Send();
	delete bitmap;
}


void
uSynergyInputServerDevice::ScreenActive(bool active)
{
}


BMessage*
uSynergyInputServerDevice::_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons, float x, float y) const
{
	BMessage* message = new BMessage(what);
	if (message == NULL)
		return NULL;

	if (message->AddInt64("when", when) < B_OK
	    || message->AddInt32("buttons", buttons) < B_OK
	    || message->AddFloat("x", x) < B_OK
	    || message->AddFloat("y", y) < B_OK) {
		delete message;
		return NULL;
	}

	return message;
}


void
uSynergyInputServerDevice::MouseCallback(uint16_t x, uint16_t y, int16_t wheelX,
	int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight,
	uSynergyBool buttonMiddle)
{
	static uint32_t			 oldButtons = 0, oldPressedButtons = 0;
	uint32_t			 buttons = 0;
	static uint16_t			 oldX = 0, oldY = 0, clicks = 0;
	static int16_t			oldWheelX = 0, oldWheelY = 0;
	static uint64			oldWhen = system_time();
	float				 xVal = (float)x / (float)fContext->m_clientWidth;
	float				 yVal = (float)y / (float)fContext->m_clientHeight;

	int64 timestamp = system_time();

	if (buttonLeft == USYNERGY_TRUE) {
		buttons |= 1 << 0;
	}
	if (buttonRight == USYNERGY_TRUE) {
		buttons |= 1 << 1;
	}
	if (buttonMiddle == USYNERGY_TRUE) {
		buttons |= 1 << 2;
	}

	if (buttons != oldButtons) {
		bool pressedButton = buttons > 0;
		BMessage* message = _BuildMouseMessage(pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP, timestamp, buttons, xVal, yVal);
		if (pressedButton) {
			if ((buttons == oldPressedButtons) && ((timestamp - oldWhen) < 500000))
				clicks++;
			else
				clicks = 1;
			message->AddInt32("clicks", clicks);
			oldWhen = timestamp;
			oldPressedButtons = buttons;
		} else
			clicks = 1;

		if (message != NULL)
			EnqueueMessage(message);

		oldButtons = buttons;
	}

	if ((x != oldX) || (y != oldY)) {
		BMessage* message = _BuildMouseMessage(B_MOUSE_MOVED, timestamp, buttons, xVal, yVal);
		if (message != NULL)
			EnqueueMessage(message);
		oldX = x;
		oldY = y;
	}

	if (wheelX != 0 || wheelY != 0) {
		BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
		if (message != NULL) {
			if (message->AddInt64("when", timestamp) == B_OK
				&& message->AddFloat("be:wheel_delta_x",
					(oldWheelX - wheelX) / 120) == B_OK
				&& message->AddFloat("be:wheel_delta_y",
					(oldWheelY - wheelY) / 120) == B_OK)
				EnqueueMessage(message);
			else
				delete message;
		}
		oldWheelX = wheelX;
		oldWheelY = wheelY;
	}
}


void
uSynergyInputServerDevice::KeyboardCallback(uint16_t scancode,
	uint16_t _modifiers, bool isKeyDown, bool isKeyRepeat)
{
	static uint32 lastScanCode = 0;
	static uint32 repeatCount = 1;
	static uint8 states[16];

	int64 timestamp = system_time();

	uint32_t keycode = 0;

	// XXX: This is a dirty hack.
	// See https://github.com/synergy/synergy/issues/4640
	if (fServerKeymap == "X11") {
		if (scancode > 0 && scancode < sizeof(kXKeycodeMap)/sizeof(uint32))
			keycode = kXKeycodeMap[scancode - 1];
		else {
			scancode = (uint8)(scancode | 0x80);
			if (scancode > 0 && scancode < sizeof(kXKeycodeMap)/sizeof(uint32))
				keycode = kXKeycodeMap[scancode - 1];
		}
	} else {
		if (scancode > 0 && scancode < sizeof(kATKeycodeMap)/sizeof(uint32))
			keycode = kATKeycodeMap[scancode - 1];
		else {
			scancode = (uint8)(scancode | 0x80);
			if (scancode > 0 && scancode < sizeof(kATKeycodeMap)/sizeof(uint32))
				keycode = kATKeycodeMap[scancode - 1];
		}
	}

	TRACE("synergy: scancode = 0x%02x, keycode = 0x%x\n", scancode, keycode);

	if (keycode < 256) {
		if (isKeyDown)
			states[(keycode) >> 3] |= (1 << (7 - (keycode & 0x7)));
		else
			states[(keycode) >> 3] &= (!(1 << (7 - (keycode & 0x7))));
	}

#if false
	if (isKeyDown && keycode == 0x34 // DELETE KEY
		&& (states[fCommandKey >> 3] & (1 << (7 - (fCommandKey & 0x7))))
		&& (states[fControlKey >> 3] & (1 << (7 - (fControlKey & 0x7))))) {
		TRACE("synergy: TeamMonitor called\n");
	}
#endif

	uint32 modifiers = 0;

	if (_modifiers & USYNERGY_MODIFIER_SHIFT)
		modifiers |= B_SHIFT_KEY | B_LEFT_SHIFT_KEY;
	if (_modifiers & USYNERGY_MODIFIER_CTRL)
		modifiers |= B_CONTROL_KEY | B_LEFT_CONTROL_KEY;
	if (_modifiers & USYNERGY_MODIFIER_ALT)
		modifiers |= B_COMMAND_KEY | B_LEFT_COMMAND_KEY;
	if (_modifiers & USYNERGY_MODIFIER_META)
		modifiers |= B_MENU_KEY;
	if (_modifiers & USYNERGY_MODIFIER_WIN)
		modifiers |= B_OPTION_KEY | B_LEFT_OPTION_KEY;
	if (_modifiers & USYNERGY_MODIFIER_ALT_GR)
		modifiers |= B_RIGHT_OPTION_KEY | B_OPTION_KEY;
	if (_modifiers & USYNERGY_MODIFIER_CAPSLOCK)
		modifiers |= B_CAPS_LOCK;
	if (_modifiers & USYNERGY_MODIFIER_NUMLOCK)
		modifiers |= B_NUM_LOCK;
	if (_modifiers & USYNERGY_MODIFIER_SCROLLOCK)
		modifiers |= B_SCROLL_LOCK;

	//bool isLock
	//	= (modifiers & (B_CAPS_LOCK | B_NUM_LOCK | B_SCROLL_LOCK)) != 0;
	if (!isKeyRepeat) {
		if (fModifiers != modifiers) {
			BMessage* message = new BMessage(B_MODIFIERS_CHANGED);
			if (message == NULL)
				return;

			TRACE("synergy: modifiers: 0x%04" B_PRIx32 " & 0x%04" B_PRIx32 "\n", modifiers, fModifiers);

			if (isKeyDown)
				modifiers |= fModifiers;
			else
				modifiers &= ~fModifiers;

			TRACE("synergy: modifiers changed: 0x%04" B_PRIx32 " => 0x%04" B_PRIx32 "\n", fModifiers, modifiers);

			message->AddInt64("when", timestamp);
			message->AddInt32("be:old_modifiers", fModifiers);
			message->AddInt32("modifiers", modifiers);
			message->AddData("states", B_UINT8_TYPE, states, 16);

			fModifiers = modifiers;

			if (EnqueueMessage(message) != B_OK)
				delete message;
		}
	}

	if (scancode == 0)
		return;

	BMessage* msg = new BMessage;
	if (msg == NULL)
		return;

	char* string = NULL;
	char* rawString = NULL;
	int32 numBytes = 0, rawNumBytes = 0;
	fKeymap.GetChars(keycode, fModifiers, 0, &string, &numBytes);
	fKeymap.GetChars(keycode, 0, 0, &rawString, &rawNumBytes);

	if (numBytes > 0)
		msg->what = isKeyDown ? B_KEY_DOWN : B_KEY_UP;
	else
		msg->what = isKeyDown ? B_UNMAPPED_KEY_DOWN : B_UNMAPPED_KEY_UP;

	msg->AddInt64("when", timestamp);
	msg->AddInt32("key", keycode);
	msg->AddInt32("modifiers", fModifiers);
	msg->AddData("states", B_UINT8_TYPE, states, 16);
	if (numBytes > 0) {
		for (int i = 0; i < numBytes; i++) {
			TRACE("%02x:", (int8)string[i]);
			msg->AddInt8("byte", (int8)string[i]);
		}
		TRACE("\n");
		msg->AddData("bytes", B_STRING_TYPE, string, numBytes + 1);

		if (rawNumBytes <= 0) {
			rawNumBytes = 1;
			delete[] rawString;
			rawString = string;
		} else
			delete[] string;

		if (isKeyDown && isKeyRepeat) {
			repeatCount++;
			msg->AddInt32("be:key_repeat", repeatCount);
		} else
			repeatCount = 1;
	} else
		delete[] string;

	if (rawNumBytes > 0)
		msg->AddInt32("raw_char", (uint32)((uint8)rawString[0] & 0x7f));

	delete[] rawString;

	if (msg != NULL && EnqueueMessage(msg) != B_OK)
		delete msg;

	lastScanCode = isKeyDown ? scancode : 0;
}


void
uSynergyInputServerDevice::JoystickCallback(uint8_t joyNum, uint16_t buttons, int8_t leftStickX, int8_t leftStickY, int8_t rightStickX, int8_t rightStickY)
{
}


void
uSynergyInputServerDevice::ClipboardCallback(enum uSynergyClipboardFormat format, const uint8_t *data, uint32_t size)
{
	if (format != USYNERGY_CLIPBOARD_FORMAT_TEXT)
		return;

	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage *clip = be_clipboard->Data();
		clip->AddData("text/plain", B_MIME_TYPE, data, size);
		status_t result = be_clipboard->Commit();
		if (result != B_OK)
			TRACE("synergy: failed to commit data to clipboard\n");

		be_clipboard->Unlock();
	} else {
		TRACE("synergy: could not lock clipboard\n");
	}
}


extern "C" BInputServerDevice*
instantiate_input_device()
{
	return new (std::nothrow)uSynergyInputServerDevice;
}

