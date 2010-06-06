#!/bin/bash

# Make sure to adjust the QTDIR variable to where you have Qt's libraries installed.
# install_name_tool will _silently_ fail if the path is incorrect. Also, it will
# _not_ normalize double slashes.
APPNAME=QSynergy
QTDIR=$HOME/sw/qt/lib
TMPBUNDLE=tmpbundledir
PLATFORMS="ppc x86"

cd $(dirname $0)/..

rm -rf ${APPNAME}.app
make distclean
qmake -spec macx-g++ "CONFIG+=release" "CONFIG-=debug" "CONFIG+=${PLATFORMS}" "QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk"
make
mkdir ${APPNAME}.app/Contents/Frameworks
 
cp -R ${QTDIR}/QtCore.framework ${APPNAME}.app/Contents/Frameworks
cp -R ${QTDIR}/QtGui.framework ${APPNAME}.app/Contents/Frameworks
cp -R ${QTDIR}/QtNetwork.framework ${APPNAME}.app/Contents/Frameworks

rm -rf ${APPNAME}.app/Contents/Frameworks/Qt*.framework/Versions/4/Qt*_debug
rm -rf ${APPNAME}.app/Contents/Frameworks/Qt*.framework/Versions/4/Headers
  
install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore ${APPNAME}.app/Contents/Frameworks/QtCore.framework/Versions/4/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui ${APPNAME}.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -id @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork ${APPNAME}.app/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork
   
install_name_tool -change ${QTDIR}/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore ${APPNAME}.app/Contents/MacOs/$APPNAME
install_name_tool -change ${QTDIR}/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui ${APPNAME}.app/Contents/MacOs/$APPNAME
install_name_tool -change ${QTDIR}/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork ${APPNAME}.app/Contents/MacOs/$APPNAME
    
install_name_tool -change ${QTDIR}/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore ${APPNAME}.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -change ${QTDIR}/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore ${APPNAME}.app/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork

mkdir ${TMPBUNDLE}
mv ${APPNAME}.app ${TMPBUNDLE}
cp COPYING README ${TMPBUNDLE}
hdiutil create -ov -srcfolder ${TMPBUNDLE} -format UDBZ -volname "$APPNAME" "${APPNAME}.dmg"
hdiutil internet-enable -yes "${APPNAME}.dmg"
mv ${TMPBUNDLE}/${APPNAME}.app .
rm -rf ${TMPBUNDLE}
