/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "server/ClientProxy1_7.h"
#include "server/Server.h"
#include "synergy/AppUtil.h"
#include "synergy/ProtocolUtil.h"
#include "base/TMethodEventJob.h"
#include "base/Log.h"

//
// ClientProxy1_7
//

ClientProxy1_7::ClientProxy1_7(const String& name, synergy::IStream* stream, Server* server, IEventQueue* events) :
    ClientProxy1_6(name, stream, server, events),
    m_events(events)
{

}

ClientProxy1_7::~ClientProxy1_7()
{

}

bool
ClientProxy1_7::parseMessage(const UInt8* code)
{
    //if (memcmp(code, kMsgLanguageList, 4) == 0) {
    //    langInfoReceived();
    //}
    //else {
        return ClientProxy1_6::parseMessage(code);
    //}

    return true;
}

void
ClientProxy1_7::secureInputNotification(const String& app)
{
    LOG((CLOG_DEBUG2 "send secure input notification to \"%s\" %s", getName().c_str(), app.c_str()));
    ProtocolUtil::writef(getStream(), kMsgDSecureInputNotification, app.c_str());
}

/*
void
ClientProxy1_7::langInfoReceived()
{
    String clientLayoutList = "";
    ProtocolUtil::readf(getStream(), kMsgLanguageList + 4, &clientLayoutList);

    String missedLanguages;
    String supportedLanguages;
    auto localLayouts = AppUtil::instance().getKeyboardLayoutList();
    for(int i = 0; i <= (int)clientLayoutList.size() - 2; i +=2) {
        auto layout = clientLayoutList.substr(i, 2);
        if (std::find(localLayouts.begin(), localLayouts.end(), layout) == localLayouts.end()) {
            missedLanguages += layout;
            missedLanguages += ' ';
        }
        else {
            supportedLanguages += layout;
            supportedLanguages += ' ';
        }
    }

    if(!supportedLanguages.empty()) {
        LOG((CLOG_DEBUG "Supported client languages: %s", supportedLanguages.c_str()));
    }

    if(!missedLanguages.empty()) {
        AppUtil::instance().showMessageBox("Language synchronization error",
                                           String("This languages are required for server proper work: ") + missedLanguages);
    }
}
*/
