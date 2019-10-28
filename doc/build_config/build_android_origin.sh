PREFIX=/Users/wangzhumo/Develop/Media/ffmpeg/android

#设置你自己的NDK位置
NDK_HOME=/Users/wangzhumo/Library/Android/ndk-r10e
#设置你自己的平台，这上Mac上的，linux换成linux-x86_64
NDK_HOST_PLATFORM=darwin-x86_64
#设置版本
SYSROOT=$NDK_HOME/platforms/android-19/arch-arm/  

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
    "

function build_android {
    ./configure \
    --libdir=${PREFIX}/libs/armeabi-v7a \
    --incdir=${PREFIX}/includes/armeabi-v7a \
    --pkgconfigdir=${PREFIX}/pkgconfig/armeabi-v7a \
    --arch=arm \
    --cpu=armv7-a \
    --cross-prefix="${NDK_HOME}/toolchains/arm-linux-androideabi-4.9/prebuilt/${NDK_HOST_PLATFORM}/bin/arm-linux-androideabi-" \
    --sysroot=$SYSROOT \
    --extra-cflags="-march=armv7-a -mfloat-abi=softfp -mfpu=neon" \
    --extra-ldexeflags=-pie \
    ${COMMON_OPTIONS}
    make clean
    make -j8 && make install
};

build_android
