#!/bin/bash
OUTDIR=$1
CURRENTDIR=`pwd`
DEVROOT=/Applications/Xcode.app/Contents/Developer
MAC_VER=MacOSX10.8.sdk
IOS_VER=iPhoneOS6.0.sdk
SIM_VER=iPhoneSimulator6.0.sdk
TARGET=libluajit.a

if [ -e "$OUTDIR/$TARGET" ]; then
	echo "luajit universal binary already has built. please remove manually for rebuild"
	exit
fi

CROSS=$DEVROOT/Toolchains/XcodeDefault.xctoolchain/usr/bin/
MAKE=$DEVROOT/usr/bin/make
MAC_SDKROOT=$DEVROOT/Platforms/MacOSX.platform/Developer
IOS_SDKROOT=$DEVROOT/Platforms/iPhoneOS.platform/Developer
SIM_SDKROOT=$DEVROOT/Platforms/iPhoneSimulator.platform/Developer
HOST_CC="${CROSS}cc -arch i386 -isysroot $MAC_SDKROOT/SDKs/$MAC_VER"
TARGET_COFLAGS='-I../../../../../../../../src/ -include zlcore/zl_replace.h'

function validate {
	if [ -e "$1/SDKs/$2" ]; then
		echo $2
	else
		sdks=`find $1/SDKs/ -maxdepth 1 -regex .*\.sdk$`
		for sdk in ${sdks[@]}
		do
			echo `basename $sdk`
		done
	fi
}

IOS_VER=$(validate $IOS_SDKROOT $IOS_VER)
SIM_VER=$(validate $SIM_SDKROOT $SIM_VER)

echo "iOS: use $IOS_VER"
echo "sim: use $SIM_VER"

cd $2 &&
# patch luajit to use zlcore
patch -p1 < $CURRENTDIR/build_luajit.patch && \
$MAKE clean && $MAKE CC=cc CROSS=$CROSS HOST_CC="$HOST_CC" TARGET_SYS=iOS \
	TARGET_FLAGS="-arch armv7  -isysroot $IOS_SDKROOT/SDKs/$IOS_VER" TARGET_COFLAGS=$COFLAGS && \
	cp -p src/libluajit.a $OUTDIR/libluajit-armv7.a && \
$MAKE clean && $MAKE CC=cc CROSS=$CROSS HOST_CC="$HOST_CC" TARGET_SYS=iOS \
	TARGET_FLAGS="-arch armv7s -isysroot $IOS_SDKROOT/SDKs/$IOS_VER" TARGET_COFLAGS=$COFLAGS && \
	cp -p src/libluajit.a $OUTDIR/libluajit-armv7s.a && \
$MAKE clean && $MAKE CC=cc CROSS=$CROSS HOST_CC="$HOST_CC" TARGET_SYS=iOS \
	TARGET_FLAGS="-arch i386   -isysroot $SIM_SDKROOT/SDKs/$SIM_VER" TARGET_COFLAGS=$COFLAGS && \
	cp -p src/libluajit.a $OUTDIR/libluajit-i386.a && \
# rollback patched files
git checkout src/Makefile && \
lipo -create -output $OUTDIR/$TARGET \
	$OUTDIR/libluajit-arm*.a $OUTDIR/libluajit-i386.a && \
rm $OUTDIR/libluajit-arm*.a $OUTDIR/libluajit-i386.a
