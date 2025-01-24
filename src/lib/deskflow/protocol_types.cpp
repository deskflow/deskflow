/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/protocol_types.h"

const char *const kSynergyProtocolName = "Synergy";
const char *const kBarrierProtocolName = "Barrier";

// The protocol name string within the hello and hello back messages must be
// 7 chars for backward compatibility (Synergy and Barrier are 7 chars).
const char *const kMsgHello = "%7s%2i%2i";
const char *const kMsgHelloArgs = "%2i%2i";
const char *const kMsgHelloBack = "%7s%2i%2i%s";
const char *const kMsgHelloBackArgs = "%2i%2i%s";
const char *const kMsgCNoop = "CNOP";
const char *const kMsgCClose = "CBYE";
const char *const kMsgCEnter = "CINN%2i%2i%4i%2i";
const char *const kMsgCLeave = "COUT";
const char *const kMsgCClipboard = "CCLP%1i%4i";
const char *const kMsgCScreenSaver = "CSEC%1i";
const char *const kMsgCResetOptions = "CROP";
const char *const kMsgCInfoAck = "CIAK";
const char *const kMsgCKeepAlive = "CALV";
const char *const kMsgDKeyDownLang = "DKDL%2i%2i%2i%s";
const char *const kMsgDKeyDown = "DKDN%2i%2i%2i";
const char *const kMsgDKeyDown1_0 = "DKDN%2i%2i";
const char *const kMsgDKeyRepeat = "DKRP%2i%2i%2i%2i%s";
const char *const kMsgDKeyRepeat1_0 = "DKRP%2i%2i%2i";
const char *const kMsgDKeyUp = "DKUP%2i%2i%2i";
const char *const kMsgDKeyUp1_0 = "DKUP%2i%2i";
const char *const kMsgDMouseDown = "DMDN%1i";
const char *const kMsgDMouseUp = "DMUP%1i";
const char *const kMsgDMouseMove = "DMMV%2i%2i";
const char *const kMsgDMouseRelMove = "DMRM%2i%2i";
const char *const kMsgDMouseWheel = "DMWM%2i%2i";
const char *const kMsgDMouseWheel1_0 = "DMWM%2i";
const char *const kMsgDClipboard = "DCLP%1i%4i%1i%s";
const char *const kMsgDInfo = "DINF%2i%2i%2i%2i%2i%2i%2i";
const char *const kMsgDSetOptions = "DSOP%4I";
const char *const kMsgDFileTransfer = "DFTR%1i%s";
const char *const kMsgDDragInfo = "DDRG%2i%s";
const char *const kMsgDSecureInputNotification = "SECN%s";
const char *const kMsgDLanguageSynchronisation = "LSYN%s";
const char *const kMsgQInfo = "QINF";
const char *const kMsgEIncompatible = "EICV%2i%2i";
const char *const kMsgEBusy = "EBSY";
const char *const kMsgEUnknown = "EUNK";
const char *const kMsgEBad = "EBAD";
