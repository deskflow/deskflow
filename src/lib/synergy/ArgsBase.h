/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2020 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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


#ifndef SYNERGY_CORE_ARGSBASE_H
#define SYNERGY_CORE_ARGSBASE_H

#include "base/String.h"

namespace lib {
    namespace synergy {
        /**
         * @brief This is the base Argument class that will store the generic
         *        arguments passed into the applications this will be derived
         *        from and expanded to include application specific arguments
         */
        class ArgsBase {
        public:
            ArgsBase() = default;
            virtual ~ArgsBase();

            /// @brief This sets the type of the derived class
            enum Type { kBase, kServer, kClient };

            Type                 m_classType         = kBase;      /// @brief Stores what type of object this is

            bool                 m_daemon            = true;       /// @brief Should run as a daemon
            bool                 m_backend           = false;      /// @brief //TODO Unsure what this is used for
            bool                 m_restartable       = true;       /// @brief Should the app restart automatically
            bool                 m_noHooks           = false;      /// @brief Should the app use hooks
            const char*          m_pname             = nullptr;    /// @brief The filename of the running process
            const char*          m_logFilter         = nullptr;    /// @brief The logging level of the application
            const char*          m_logFile           = nullptr;    /// @brief The full path to the logfile
            const char*          m_display           = nullptr;    /// @brief Contains the X-Server display to use
            String               m_name;                           /// @brief The name of the current computer
            bool                 m_disableTray       = false;      /// @brief Should the app add a tray icon
            bool                 m_enableIpc         = false;      /// @brief Tell the client to talk through IPC to the daemon
            bool                 m_enableDragDrop    = false;      /// @brief Should drag drop support be enabled

            bool                 m_shouldExit        = false;      /// @brief Will cause the application to exit when set to true
            String               m_synergyAddress;                 /// @brief Bind to this address //TODO This really should be a ServerArgs
            bool                 m_enableCrypto      = false;      /// @brief Should the connections be TLS encrypted
            String               m_profileDirectory;               /// @brief The profile DIR to use for the application
            String               m_pluginDirectory;                /// @brief //TODO Plugins? Get set in ARCH but doesn't seem to get used
            String               m_tlsCertFile;                    /// @brief Contains the location of the TLS certificate file

#if SYSAPI_WIN32
            bool                 m_debugServiceWait  = false;
            bool                 m_pauseOnExit       = false;
            bool                 m_stopOnDeskSwitch  = false;
#endif
#if WINAPI_XWINDOWS
            bool                 m_disableXInitThreads   = false;
#endif

        protected:

            /// @brief deletes pointers and sets the value to null
            template<class T> static inline void destroy(T*& p) { delete p; p = 0; }

        };
    }
}

#endif //SYNERGY_CORE_ARGSBASE_H