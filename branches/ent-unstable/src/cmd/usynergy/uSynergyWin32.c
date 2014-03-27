/*
uSynergy client -- Implementation for the embedded Synergy client library
  version 1.0.0, July 7th, 2012

Copyright (c) 2012 Nick Bolton

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "uSynergy.h"

#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

uSynergyBool connect(uSynergyCookie cookie);
uSynergyBool send(uSynergyCookie cookie, const uint8_t *buffer, int length);
uSynergyBool receive(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength);
void sleep(uSynergyCookie cookie, int timeMs);
uint32_t getTime();
void trace(uSynergyCookie cookie, const char *text);
void screenActive(uSynergyCookie cookie, uSynergyBool active);
void mouse(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX,
	int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight,
	uSynergyBool buttonMiddle);
void keyboard(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat);
void joystick(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons,
	int8_t leftStickX, int8_t leftStickY, int8_t rightStickX,
	int8_t rightStickY);
void clipboard(uSynergyCookie cookie, enum uSynergyClipboardFormat format,
	const uint8_t *data, uint32_t size);

// TODO: implement callbacks.
int main(char* argv, int argc)
{
	uSynergyContext context;
	uSynergyInit(&context);

	context.m_connectFunc = &connect;
	context.m_sendFunc = &send;
	context.m_receiveFunc = &receive;
	context.m_sleepFunc = &sleep;
	context.m_getTimeFunc = &getTime;
	context.m_traceFunc = &trace;
	context.m_screenActiveCallback = &screenActive;
	context.m_mouseCallback = &mouse;
	context.m_keyboardCallback = &keyboard;
	context.m_joystickCallback = &joystick;
	context.m_clipboardCallback = &clipboard;
	
	for(;;) {
		uSynergyUpdate(&context);
	}
}

uSynergyBool connect(uSynergyCookie cookie)
{
	printf("connect\n");
	return USYNERGY_TRUE;
}

uSynergyBool send(uSynergyCookie cookie, const uint8_t *buffer, int length)
{
	printf("send\n");
	return USYNERGY_TRUE;
}

uSynergyBool receive(uSynergyCookie cookie, uint8_t *buffer, int maxLength, int* outLength)
{
	printf("receive\n");
	return USYNERGY_TRUE;
}

void sleep(uSynergyCookie cookie, int timeMs)
{
	printf("sleep, timeMs=%d\n", timeMs);
	Sleep(timeMs);
}

uint32_t getTime()
{
	printf("getTime\n");
	return 0;
}

void trace(uSynergyCookie cookie, const char *text)
{
	printf("%s\n", text);
}

void screenActive(uSynergyCookie cookie, uSynergyBool active)
{
	printf("screenActive, active=%d\n", active);
}

void mouse(uSynergyCookie cookie, uint16_t x, uint16_t y, int16_t wheelX,
	int16_t wheelY, uSynergyBool buttonLeft, uSynergyBool buttonRight,
	uSynergyBool buttonMiddle)
{
	printf("mouse, pos=%d,%d\n", x, y);
}

void keyboard(uSynergyCookie cookie, uint16_t key, uint16_t modifiers, uSynergyBool down, uSynergyBool repeat)
{
	printf("keyboard, key=%d down=%d repeat=%d\n", key, down, repeat);
}

void joystick(uSynergyCookie cookie, uint8_t joyNum, uint16_t buttons,
	int8_t leftStickX, int8_t leftStickY, int8_t rightStickX,
	int8_t rightStickY)
{
	printf("joystick, left=%d,%d right=%d,%d\n", leftStickX, leftStickY,
		rightStickX, rightStickY);
}

void clipboard(uSynergyCookie cookie, enum uSynergyClipboardFormat format,
	const uint8_t *data, uint32_t size)
{
	printf("clipboard, size=%d\n", size);
}
