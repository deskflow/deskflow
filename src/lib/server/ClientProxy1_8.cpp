/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 - 2021 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClientProxy1_8.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "deskflow/DragInformation.h"
#include "deskflow/KeyboardLayoutManager.h"
#include "deskflow/ProtocolUtil.h"

ClientProxy1_8::ClientProxy1_8(
    const std::string &name, deskflow::IStream *adoptedStream, Server *server, IEventQueue *events
)
    : ClientProxy1_7(name, adoptedStream, server, events),
      m_events(events)
{
  synchronizeLanguages();

  m_events->addHandler(EventTypes::FileChunkSending, this, [this](const auto &e) {
    FileChunk::send(getStream(), e.getDataObject());
  });
  m_events->addHandler(EventTypes::DragInfoSending, this, [this](const auto &e) {
    const auto *data = dynamic_cast<const DragInfoEventData *>(e.getDataObject());
    if (data) {
      ProtocolUtil::writef(getStream(), kMsgDDragInfo, data->fileCount, &data->info);
    } else {
      LOG_ERR("DragInfoSending without DragInfoEventData");
    }
  });
}

ClientProxy1_8::~ClientProxy1_8()
{
  if (m_events) {
    m_events->removeHandler(EventTypes::FileChunkSending, this);
    m_events->removeHandler(EventTypes::DragInfoSending, this);
  }
}

void ClientProxy1_8::synchronizeLanguages() const
{
  deskflow::KeyboardLayoutManager layoutManager;
  auto localLayouts = layoutManager.getSerializedLocalLayouts();
  if (!localLayouts.empty()) {
    LOG_VERBOSE("send server languages to the client: %s", localLayouts.c_str());
    ProtocolUtil::writef(getStream(), kMsgDLanguageSynchronisation, &localLayouts);
  } else {
    LOG_ERR("failed to read server languages");
  }
}

void ClientProxy1_8::keyDown(KeyID key, KeyModifierMask mask, KeyButton button, const std::string &language)
{
  LOG_VERBOSE(
      "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x, layout=%s", getName().c_str(), key, mask, button,
      language.c_str()
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyDown, key, mask, button, &language);
}

void ClientProxy1_8::keyRepeat(
    KeyID key, KeyModifierMask mask, int32_t count, KeyButton button, const std::string &language
)
{
  LOG_VERBOSE(
      "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x, layout=%s", getName().c_str(), key, mask,
      count, button, language.c_str()
  );
  ProtocolUtil::writef(getStream(), kMsgDKeyRepeat, key, mask, count, button, &language);
}
