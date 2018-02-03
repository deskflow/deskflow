#!/bin/sh
if ! which magick >/dev/null 2>&1; then
    echo "Need ImageMagic for this"
    exit 10
fi
cd $(dirname $0) || exit $?
if [ ! -r barrier.png ]; then
    echo "Use inkscape (or another vector graphics editor) to create barrier.png from barrier.svg first"
    exit 10
fi
rm -rf work || exit $?
mkdir -p work || exit $?
for s in 16 24 32 48 64 128; do
    magick convert barrier.png -resize ${s}x${s} work/${s}.png || exit $?
done
magick convert work/{16,24,32,48,64,128}.png barrier.png barrier.ico || exit $?
rm -rf work
echo Done
