/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "deskflow/ProtocolUtil.h"
#include "deskflow/languages/LanguageManager.h"

#include "ClientProxy1_8.h"

ClientProxy1_8::ClientProxy1_8(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
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

void ClientProxy1_8::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &language)
{
  LOG(
      (CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x, language=%s", getName().c_str(), key,
       mask, button, language.c_str())
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyDownLang, key, mask, button, &language);
}
