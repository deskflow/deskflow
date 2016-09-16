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

#import "platform/OSXMediaKeySupport.h"
#import <Cocoa/Cocoa.h>
#import <IOKit/hidsystem/ev_keymap.h>

int convertKeyIDToNXKeyType(KeyID id)
{
	int type = -1;

	switch (id) {
	case kKeyAudioUp:
		type = NX_KEYTYPE_SOUND_UP;
		break;
	case kKeyAudioDown:
		type = NX_KEYTYPE_SOUND_DOWN;
		break;
	case kKeyBrightnessUp:
		type = NX_KEYTYPE_BRIGHTNESS_UP;
		break;
	case kKeyBrightnessDown:
		type = NX_KEYTYPE_BRIGHTNESS_DOWN;
		break;
	case kKeyAudioMute:
		type = NX_KEYTYPE_MUTE;
		break;
	case kKeyEject:
		type = NX_KEYTYPE_EJECT;
		break;
	case kKeyAudioPlay:
		type = NX_KEYTYPE_PLAY;
		break;
	case kKeyAudioNext:
		type = NX_KEYTYPE_NEXT;
		break;
	case kKeyAudioPrev:
		type = NX_KEYTYPE_PREVIOUS;
		break;
	default:
		break;
	}
	
	return type;
}

static KeyID
convertNXKeyTypeToKeyID(uint32_t const type)
{
	KeyID id = 0;

	switch (type) { 
	case NX_KEYTYPE_SOUND_UP:
		id = kKeyAudioUp;
		break;
	case NX_KEYTYPE_SOUND_DOWN:
		id = kKeyAudioDown;
		break;
	case NX_KEYTYPE_MUTE:
		id = kKeyAudioMute;
		break;
	case NX_KEYTYPE_EJECT:
		id = kKeyEject;
		break;
	case NX_KEYTYPE_PLAY:
		id = kKeyAudioPlay;
		break;
	case NX_KEYTYPE_FAST:
	case NX_KEYTYPE_NEXT:
		id = kKeyAudioNext;
		break;
	case NX_KEYTYPE_REWIND:
	case NX_KEYTYPE_PREVIOUS:
		id = kKeyAudioPrev;
		break;
	default:
		break;
	}

	return id;
}

bool
isMediaKeyEvent(CGEventRef event) {
	NSEvent* nsEvent = nil;
	@try {
		nsEvent = [NSEvent eventWithCGEvent: event];
		if ([nsEvent subtype] != 8) {
			return false;
		}
		uint32_t const nxKeyId = ([nsEvent data1] & 0xFFFF0000) >> 16;
		if (convertNXKeyTypeToKeyID (nxKeyId)) {
			return true;
		}
	} @catch (NSException* e) {
	}
	return false;
}

bool
getMediaKeyEventInfo(CGEventRef event, KeyID* const keyId, 
					 bool* const down, bool* const isRepeat) {
	NSEvent* nsEvent = nil;
	@try {
		nsEvent = [NSEvent eventWithCGEvent: event];
	} @catch (NSException* e) {
		return false;
	}
	if (keyId) {
		*keyId = convertNXKeyTypeToKeyID (([nsEvent data1] & 0xFFFF0000) >> 16);
	}
	if (down) {
		*down = !([nsEvent data1] & 0x100);
	}
	if (isRepeat) {
		*isRepeat = [nsEvent data1] & 0x1;
	}
	return true;
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
