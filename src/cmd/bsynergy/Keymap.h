/*
 * Copyright 2004-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef KEYMAP_H
#define KEYMAP_H


#include <Keymap.h>
#include <Entry.h>


class Keymap : public BKeymap {
public:
								Keymap();
								~Keymap();

			void				DumpKeymap();

			status_t			RetrieveCurrent();
};


#endif	// KEYMAP_H
