/*
 * Copyright (c)
 *      2014  Ed Robbins <edd.robbins@gmail.com>
 *      2014  Jessica Hamilton <jessica.l.hamilton@gmail.com>
 *
 * Based on MouseInputDevice.h by
 *    Stefano Ceccherini Copyright 2004-2008, Haiku
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

#ifndef USYNERGY_H
#define USYNERGY_H

#include <InputServerDevice.h>
#include <InterfaceDefs.h>
#include <Locker.h>

#include <ObjectList.h>

#include "Keymap.h"
#include "uSynergy.h"


class uSynergyInputServerDevice : public BHandler, public BInputServerDevice {
	public:
							uSynergyInputServerDevice();
	virtual					~uSynergyInputServerDevice();

	virtual status_t		InitCheck();

	virtual status_t		Control(const char* name, void* cookie,
								uint32 command, BMessage* message);
	virtual status_t		Start(const char* name, void* cookie);
	virtual status_t		Stop(const char* name, void* cookie);
	virtual status_t		SystemShuttingDown();

	virtual	void			MessageReceived(BMessage* message);

	// Synergy Hooks
		bool				Connect();
		bool				Send(const uint8_t* buffer, int32_t length);
		bool				Receive(uint8_t* buffer, int maxLength,
								int* outLength);
		void				Trace(const char* text);
		void				ScreenActive(bool active);
		void				MouseCallback(uint16_t x, uint16_t y,
								int16_t wheelX, int16_t wheelY,
								uSynergyBool buttonLeft,
								uSynergyBool buttonRight,
								uSynergyBool buttonMiddle);
		void				KeyboardCallback(uint16_t key, uint16_t modifiers,
								bool isKeyDown, bool isKeyRepeat);
		void				JoystickCallback(uint8_t joyNum, uint16_t buttons,
								int8_t leftStickX, int8_t leftStickY,
								int8_t rightStickX, int8_t rightStickY);
		void				ClipboardCallback(enum uSynergyClipboardFormat format,
								const uint8_t* data, uint32_t size);

	private:

		BMessage*		_BuildMouseMessage(uint32 what, uint64 when,
							uint32 buttons, float x, float y) const;
		void			_UpdateSettings();
	static status_t		_MainLoop(void* arg);

		bool				threadActive;
		thread_id			uSynergyThread;
		uSynergyContext*	fContext;
		int					fSocket;

		uint32				fModifiers;
		uint32				fCommandKey;
		uint32				fControlKey;
		char*				fFilename;
		bool				fEnableSynergy;
		BString				fServerKeymap;
		BString				fServerAddress;

	volatile bool			fUpdateSettings;

		Keymap				fKeymap;
		BLocker				fKeymapLock;
};


extern "C" BInputServerDevice* instantiate_input_device();

#endif /* USYNERGY_H */
