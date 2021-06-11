#!/bin/bash

if [ $OSTYPE = "cygwin" ] ; then
	SCRIPT=$(cygpath -u -a "$0")
	cd $(dirname "$SCRIPT")
fi

LANGDIRS=$(ls -d -- */)

for DIR in $LANGDIRS ; do
	pushd "./${DIR}/LC_MESSAGES" || continue

	for POFILE in $(ls *.po) ; do
		MOFILE="$(basename "$POFILE" .po).mo"
		msgfmt -o "$MOFILE" "$POFILE"
	done
	
	popd
done

echo "premi un tasto..."
read ANS