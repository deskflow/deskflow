cd ${CIRRUS_WORKING_DIR}

source ./build/version

SYNERGY_VERSION="$SYNERGY_VERSION_MAJOR.$SYNERGY_VERSION_MINOR.$SYNERGY_VERSION_PATCH"
SYNERGY_REVISION=`git rev-parse --short=8 HEAD`
SYNERGY_DEB_VERSION="${SYNERGY_VERSION}.${SYNERGY_VERSION_STAGE}~b${BUILD_NUMBER}+${SYNERGY_REVISION}"

dch --create --package "synergy" --controlmaint --distribution unstable --newversion $SYNERGY_DEB_VERSION "Initial release"

export DEB_BUILD_OPTIONS="parallel=4"
debuild --preserve-envvar SYNERGY_* --preserve-envvar GIT_COMMIT --preserve-envvar BUILD_NUMBER