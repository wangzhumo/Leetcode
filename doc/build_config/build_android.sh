COMMON_OPTIONS="\
    --target-os=android \
    --disable-static \
    --enable-shared \
    --enable-small \
    --disable-programs \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-doc \
    --disable-symver \
    --disable-asm \
    --enable-libx264 \
	--enable-gpl \
	--enable-nonfree  \
    --enable-libopus \
    --enable-libtheora \
    --enable-libvorbis \
    --enable-libfdk-aac \
    --enable-libmp3lame \
    --enable-libspeex \
    "


ADDI_CFLAGS="-marm"
#API版本
API=19
PLATFORM=arm-linux-androideabi
CPU=armv7-a
#设置你自己的NDK位置
NDK=/Users/wangzhumo/Library/Android/ndk-r17c
SYSROOT=$NDK/platforms/android-$API/arch-arm/
ISYSROOT=$NDK/sysroot
ASM=$ISYSROOT/usr/include/$PLATFORM
TOOLCHAIN=$NDK/toolchains/$PLATFORM-4.9/prebuilt/darwin-x86_64

#输出目录
OUTPUT=/Users/wangzhumo/Develop/Media/ffmpeg/android
function build_android
{
./configure \
--prefix=$OUTPUT \
--enable-shared \
--disable-static \
--disable-doc \
--disable-ffmpeg \
--disable-ffplay \
--disable-ffprobe \
--disable-avdevice \
--disable-doc \
--disable-symver \
--cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
--target-os=android \
--arch=arm \
--enable-cross-compile \
--sysroot=$SYSROOT \
--extra-cflags="-I$ASM -isysroot $ISYSROOT -Os -fpic -marm -mfpu=neon" \
--extra-ldflags="-marm" \
$ADDITIONAL_CONFIGURE_FLAG
  make clean
  make 
  make install
}

build_android