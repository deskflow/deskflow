/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#import "platform/OSXMediaKeySimulator.h"

#import <Cocoa/Cocoa.h>

int convertKeyIDToNXKeyType(KeyID id)
{
	// hidsystem/ev_keymap.h
	// NX_KEYTYPE_SOUND_UP			0
	// NX_KEYTYPE_SOUND_DOWN		1
	// NX_KEYTYPE_BRIGHTNESS_UP		2
	// NX_KEYTYPE_BRIGHTNESS_DOWN	3
	// NX_KEYTYPE_MUTE				7
	// NX_KEYTYPE_EJECT				14
	// NX_KEYTYPE_PLAY				16
	// NX_KEYTYPE_NEXT				17
	// NX_KEYTYPE_PREVIOUS			18
	// NX_KEYTYPE_FAST				19
	// NX_KEYTYPE_REWIND			20

	int type = -1;
	switch (id) {
	case kKeyAudioUp:
		type = 0;
		break;
	case kKeyAudioDown:
		type = 1;
		break;
	case kKeyBrightnessUp:
		type = 2;
		break;
	case kKeyBrightnessDown:
		type = 3;
		break;
	case kKeyAudioMute:
		type = 7;
		break;
	case kKeyEject:
		type = 14;
		break;
	case kKeyAudioPlay:
		type = 16;
		break;
	case kKeyAudioNext:
		type = 17;
		break;
	case kKeyAudioPrev:
		type = 18;
		break;
	default:
		break;
	}
	
	return type;
}

bool
fakeNativeMediaKey(KeyID id)
{
	
	NSEvent* downRef = [NSEvent otherEventWithType:NSSystemDefined
					location: NSMakePoint(0, 0) modifierFlags:0xa00
					timestamp:0 windowNumber:0 context:0 subtype:8
					data1:(convertKeyIDToNXKeyType(id) << 16) | ((0xa) << 8)
					data2:-1];
	CGEventRef downEvent = [downRef CGEvent];
	
	NSEvent* upRef = [NSEvent otherEventWithType:NSSystemDefined
					location: NSMakePoint(0, 0) modifierFlags:0xa00
					timestamp:0 windowNumber:0 context:0 subtype:8
					data1:(convertKeyIDToNXKeyType(id) << 16) | ((0xb) << 8)
					data2:-1];
	CGEventRef upEvent = [upRef CGEvent];
	
	CGEventPost(0, downEvent);
	CGEventPost(0, upEvent);
	
	return true;
}
