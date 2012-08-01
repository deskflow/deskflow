/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ProtocolTypes.h"

const char*				kMsgHello			= "Synergy%2i%2i";
const char*				kMsgHelloBack		= "Synergy%2i%2i%s";
const char*				kMsgCNoop 			= "CNOP";
const char*				kMsgCClose 			= "CBYE";
const char*				kMsgCEnter 			= "CINN%2i%2i%4i%2i";
const char*				kMsgCLeave 			= "COUT";
const char*				kMsgCClipboard 		= "CCLP%1i%4i";
const char*				kMsgCScreenSaver 	= "CSEC%1i";
const char*				kMsgCResetOptions	= "CROP";
const char*				kMsgCInfoAck		= "CIAK";
const char*				kMsgCKeepAlive		= "CALV";
const char*				kMsgCGameTimingReq	= "CGRQ";
const char*				kMsgCGameTimingResp	= "CGRS%2i";
const char*				kMsgDKeyDown		= "DKDN%2i%2i%2i";
const char*				kMsgDKeyDown1_0		= "DKDN%2i%2i";
const char*				kMsgDKeyRepeat		= "DKRP%2i%2i%2i%2i";
const char*				kMsgDKeyRepeat1_0	= "DKRP%2i%2i%2i";
const char*				kMsgDKeyUp			= "DKUP%2i%2i%2i";
const char*				kMsgDKeyUp1_0		= "DKUP%2i%2i";
const char*				kMsgDMouseDown		= "DMDN%1i";
const char*				kMsgDMouseUp		= "DMUP%1i";
const char*				kMsgDMouseMove		= "DMMV%2i%2i";
const char*				kMsgDMouseRelMove	= "DMRM%2i%2i";
const char*				kMsgDMouseWheel		= "DMWM%2i%2i";
const char*				kMsgDMouseWheel1_0	= "DMWM%2i";
const char*				kMsgDClipboard		= "DCLP%1i%4i%s";
const char*				kMsgDInfo			= "DINF%2i%2i%2i%2i%2i%2i%2i";
const char*				kMsgDSetOptions		= "DSOP%4I";
const char*				kMsgDGameButtons	= "DGBT%1i%2i";
const char*				kMsgDGameSticks		= "DGST%1i%2i%2i%2i%2i";
const char*				kMsgDGameTriggers	= "DGTR%1i%1i%1i";
const char*				kMsgDGameFeedback	= "DGFB%1i%2i%2i";
const char*				kMsgQInfo			= "QINF";
const char*				kMsgEIncompatible	= "EICV%2i%2i";
const char*				kMsgEBusy 			= "EBSY";
const char*				kMsgEUnknown		= "EUNK";
const char*				kMsgEBad			= "EBAD";
