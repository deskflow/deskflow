#! /bin/zsh

export SYNERGY_NO_LEGACY=1
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
mkdir -p build
pushd build
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
  -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
  ..
make synergyc synergys
zip synergy-core-macos-x64.zip bin/*
echo "::set-output name=location::build/synergy-core-macos-x64.zip"
echo "::set-output name=name::synergy-core-macos-x64.zip"
popd
