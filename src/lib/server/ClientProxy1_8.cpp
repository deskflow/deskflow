/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2021 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/languages/LanguageManager.h"

#include "ClientProxy1_8.h"

ClientProxy1_8::ClientProxy1_8(
    const String &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
)
    : ClientProxy1_7(name, adoptedStream, server, events)
{
  synchronizeLanguages();
}

void ClientProxy1_8::synchronizeLanguages() const
{
  deskflow::languages::LanguageManager languageManager;
  auto localLanguages = languageManager.getSerializedLocalLanguages();
  if (!localLanguages.empty()) {
    LOG((CLOG_DEBUG1 "send server languages to the client: %s", localLanguages.c_str()));
    ProtocolUtil::writef(getStream(), kMsgDLanguageSynchronisation, &localLanguages);
  } else {
    LOG((CLOG_ERR "failed to read server languages"));
  }
}

void ClientProxy1_8::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const String &language)
{
  LOG(
      (CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x, language=%s", getName().c_str(), key,
       mask, button, language.c_str())
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyDownLang, key, mask, button, &language);
}
