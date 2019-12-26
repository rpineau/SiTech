#!/bin/bash

mkdir -p ROOT/tmp/SiTech_X2/
cp "../SiTech.ui" ROOT/tmp/SiTech_X2/
cp "../mountlist SiTech.txt" ROOT/tmp/SiTech_X2/
cp "../build/Release/libSiTech.dylib" ROOT/tmp/SiTech_X2/


PACKAGE_NAME="SiTech_X2.pkg"
BUNDLE_NAME="org.rti-zone.SiTechX2"

if [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --sign "$installer_signature" --scripts Scripts --version 1.0 $PACKAGE_NAME
	pkgutil --check-signature ./${PACKAGE_NAME}
else
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi

rm -rf ROOT
