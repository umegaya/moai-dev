#!/bash/sh

pushd ../../../../3rdparty/yue/src/jni > /dev/null

echo "MY_LUAJIT_OPTIONAL_CFLAGS := '-I../../../../../../../../src/ -include zlcore/zl_replace.h'" > LuajitConfig.mk

popd > /dev/null

