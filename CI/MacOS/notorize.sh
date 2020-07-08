#Notarization script from https://github.com/rednoah/notarize-app/blob/master/notarize-app

BUNDLE_ID=com.symless.synergy.${SYNERGY_VERSION}.${SYNERGY_REVISION}

echo "User: ${ASC_USERNAME}"
echo "Bundle: ${BUNDLE_ID}"

# create temporary files
NOTARIZE_APP_LOG=$(mktemp -t notarize-app)
NOTARIZE_INFO_LOG=$(mktemp -t notarize-info)



# submit app for notarization
if xcrun altool --notarize-app --primary-bundle-id "$BUNDLE_ID" --username "$ASC_USERNAME" --password "${NOTORY_APP_PASSWORD}" -f "${SYNERGY_DMG_FILENAME}" > "$NOTARIZE_APP_LOG" 2>&1; then
	cat "$NOTARIZE_APP_LOG"
	RequestUUID=$(awk -F ' = ' '/RequestUUID/ {print $2}' "$NOTARIZE_APP_LOG")

	# check status periodically
	while sleep 60 && date; do
		# check notarization status
		if xcrun altool --notarization-info "$RequestUUID" --username "$ASC_USERNAME" --password "${NOTORY_APP_PASSWORD}" > "$NOTARIZE_INFO_LOG" 2>&1; then
			cat "$NOTARIZE_INFO_LOG"

			# once notarization is complete, run stapler and exit
			if ! grep -q "Status: in progress" "$NOTARIZE_INFO_LOG"; then
				xcrun stapler staple "${SYNERGY_DMG_FILENAME}"
				exit
			fi
		else
			cat "$NOTARIZE_INFO_LOG" 1>&2
			exit 1
		fi
	done
else
	cat "$NOTARIZE_APP_LOG" 1>&2
	exit 1
fi
