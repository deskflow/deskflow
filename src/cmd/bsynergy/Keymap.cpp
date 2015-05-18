/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "Keymap.h"

#include <ByteOrder.h>
#include <File.h>
#include <InputServerTypes.h>
#include <Message.h>
#include <input_globals.h>

#include <errno.h>
#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static void
print_key(char* chars, int32 offset, bool last = false)
{
	int size = chars[offset++];

	switch (size) {
		case 0:
			// Not mapped
			fputs("N/A", stdout);
			break;

		case 1:
			// single-byte UTF-8/ASCII character
			fputc(chars[offset], stdout);
			break;

		default:
		{
			// 2-, 3-, or 4-byte UTF-8 character
			char* str = new (std::nothrow) char[size + 1];
			if (str == NULL)
				break;

			strncpy(str, &(chars[offset]), size);
			str[size] = 0;
			fputs(str, stdout);
			delete [] str;
			break;
		}
	}

	if (!last)
		fputs("\t", stdout);
}


//	#pragma mark -


Keymap::Keymap()
{
	RetrieveCurrent();
}


Keymap::~Keymap()
{
}


void
Keymap::DumpKeymap()
{
	if (fKeys.version != 3)
		return;

	// Print a chart of the normal, shift, control, option, option+shift,
	// Caps, Caps+shift, Caps+option, and Caps+option+shift keys.
	puts("Key #\tn\ts\tc\to\tos\tC\tCs\tCo\tCos\n");

	for (uint8 i = 0; i < 128; i++) {
		printf(" 0x%02x\t", i);
		print_key(fChars, fKeys.normal_map[i]);
		print_key(fChars, fKeys.shift_map[i]);
		print_key(fChars, fKeys.control_map[i]);
		print_key(fChars, fKeys.option_map[i]);
		print_key(fChars, fKeys.option_shift_map[i]);
		print_key(fChars, fKeys.caps_map[i]);
		print_key(fChars, fKeys.caps_shift_map[i]);
		print_key(fChars, fKeys.option_caps_map[i]);
		print_key(fChars, fKeys.option_caps_shift_map[i], true);
		fputs("\n", stdout);
	}
}


status_t
Keymap::RetrieveCurrent()
{
	Unset();

	key_map* keys;
	_get_key_map(&keys, &fChars, (ssize_t*)&fCharsSize);
	if (!keys) {
		fprintf(stderr, "error while getting current keymap!\n");
		return B_ERROR;
	}

	memcpy(&fKeys, keys, sizeof(fKeys));
	free(keys);
	return B_OK;
}

