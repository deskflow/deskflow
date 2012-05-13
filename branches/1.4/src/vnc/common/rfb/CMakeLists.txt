# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Nick Bolton
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file COPYING that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(inc
	Blacklist.h
	CConnection.h
	CMsgHandler.h
	CMsgReader.h
	CMsgReaderV3.h
	CMsgWriter.h
	CMsgWriterV3.h
	ColourCube.h
	ColourMap.h
	ComparingUpdateTracker.h
	Configuration.h
	ConnParams.h
	CSecurity.h
	CSecurityNone.h
	CSecurityVncAuth.h
	Cursor.h
	d3des.h
	Decoder.h
	Encoder.h
	encodings.h
	Exception.h
	hextileConstants.h
	hextileDecode.h
	HextileDecoder.h
	hextileEncode.h
	HextileEncoder.h
	Hostname.h
	HTTPServer.h
	ImageGetter.h
	InputHandler.h
	KeyRemapper.h
	keysymdef.h
	Logger.h
	Logger_file.h
	Logger_stdio.h
	LogWriter.h
	msgTypes.h
	Password.h
	Pixel.h
	PixelBuffer.h
	PixelFormat.h
	RawDecoder.h
	RawEncoder.h
	Rect.h
	Region.h
	rreDecode.h
	RREDecoder.h
	rreEncode.h
	RREEncoder.h
	SConnection.h
	SDesktop.h
	secTypes.h
	ServerCore.h
	SMsgHandler.h
	SMsgReader.h
	SMsgReaderV3.h
	SMsgWriter.h
	SMsgWriterV3.h
	SSecurity.h
	SSecurityFactoryStandard.h
	SSecurityNone.h
	SSecurityVncAuth.h
	Threading.h
	Timer.h
	TransImageGetter.h
	transInitTempl.h
	transTempl.h
	TrueColourMap.h
	UpdateTracker.h
	UserPasswdGetter.h
	util.h
	VNCSConnectionST.h
	VNCServer.h
	VNCServerST.h
	zrleDecode.h
	ZRLEDecoder.h
	zrleEncode.h
	ZRLEEncoder.h
)

set(src
	d3des.c
	Blacklist.cxx
	CConnection.cxx
	CMsgHandler.cxx
	CMsgReader.cxx
	CMsgReaderV3.cxx
	CMsgWriter.cxx
	CMsgWriterV3.cxx
	ComparingUpdateTracker.cxx
	Configuration.cxx
	ConnParams.cxx
	CSecurityVncAuth.cxx
	Cursor.cxx
	Decoder.cxx
	Encoder.cxx
	encodings.cxx
	HextileDecoder.cxx
	HextileEncoder.cxx
	HTTPServer.cxx
	KeyRemapper.cxx
	Logger.cxx
	Logger_file.cxx
	Logger_stdio.cxx
	LogWriter.cxx
	Password.cxx
	PixelBuffer.cxx
	PixelFormat.cxx
	RawDecoder.cxx
	RawEncoder.cxx
	Region.cxx
	RREDecoder.cxx
	RREEncoder.cxx
	SConnection.cxx
	secTypes.cxx
	ServerCore.cxx
	SMsgHandler.cxx
	SMsgReader.cxx
	SMsgReaderV3.cxx
	SMsgWriter.cxx
	SMsgWriterV3.cxx
	SSecurityFactoryStandard.cxx
	SSecurityVncAuth.cxx
	Timer.cxx
	TransImageGetter.cxx
	UpdateTracker.cxx
	util.cxx
	VNCSConnectionST.cxx
	VNCServerST.cxx
	ZRLEDecoder.cxx
	ZRLEEncoder.cxx
)

if (WIN32)
	list(APPEND src ${inc})
endif()

set(inc
	..
	../../win
)

include_directories(${inc})
add_library(vnc_rfb STATIC ${src})
target_link_libraries(vnc_rfb
	vnc_Xregion)
