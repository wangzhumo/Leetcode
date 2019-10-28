./configure --cc=/usr/bin/clang  \
	--prefix=/usr/local/ffmpeg \
	--enable-sdl \
	--enable-libopus \
	--enable-gpl \
	--enable-nonfree \
	--enable-libfdk-aac \
	--enable-libx264 \
	--enable-libmp3lame \
	--enable-libopencore-amrnb \
	--enable-libopencore-amrwb \
	--enable-libx265 \
	--enable-filter=delogo \
	--enable-debug \
	--disable-optimizations \
	--enable-libspeex \
	--enable-videotoolbox \
	--enable-shared \
	--enable-pthreads \
	--enable-version3 \
	--enable-libtheora \
	--enable-libvorbis \
	--enable-hardcoded-tables \
	--host-cflags= \
	--host-ldflags= 

make clean
