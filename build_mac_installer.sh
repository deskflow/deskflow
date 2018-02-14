#!/bin/sh

# change this to rename the installer package
B_DMG="Barrier-v1.9.dmg"

# sanity check so we don't distribute packages full of debug symbols
echo Make sure you have built the Release version!!
echo Press Enter to continue or Ctrl+C to abort
read

cd $(dirname $0)

# remove any old copies so there's no confusion about whever this
# process completes successfully or not
rm -rf build/bundle/{bundle.dmg,$B_DMG}

B_BINARY_PATH=$(pwd)/build/bin
cd build/bundle/Barrier.app/Contents 2>/dev/null
if [ $? -ne 0 ]; then
    echo Please make sure that the build completed successfully
    echo before trying to create the installer.
    exit 1
fi

# MacOS folder holds the executables, non-system libraries,
# and the startup script
rm -rf MacOS
mkdir MacOS || exit 1
cd MacOS || exit 1

# copy all executables
cp ${B_BINARY_PATH}/* . || exit 1

# only one executable (barrier) needs non-system libraries.
# get a list of them and make local copies
B_LIBS=$(otool -XL barrier | awk '{ print $1 }' | grep -Ev '^(/usr/lib|/System)')
[ $? -ne 0 ] && exit 1
for B_SRC in $B_LIBS; do
    cp $B_SRC . || exit 1
    B_DST=@executable_path/$(basename $B_SRC)
    # adjust the executable's metadata to point to the local copy
    # rather than the system-wide copy which would only exist on
    # a development machine
    install_name_tool -change $B_SRC $B_DST  barrier || exit 1
done

# create a startup script that will change to the binary directory
# before starting barrier
printf "%s\n" "#!/bin/sh" "cd \$(dirname \$0)" "exec ./barrier" > barrier.sh
chmod +x barrier.sh

# create the DMG to be distributed in build/bundle
cd ../../..
hdiutil create -size 32m -fs HFS+ -volname "Barrier" bundle.dmg || exit 1
hdiutil attach bundle.dmg -mountpoint mnt || exit 1
cp -r Barrier.app mnt/ || exit 1
hdiutil detach mnt || exit 1
hdiutil convert bundle.dmg -format UDZO -o $B_DMG || exit 1
rm bundle.dmg

echo "Installer created successfully"
