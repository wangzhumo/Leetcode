//
// Created by 王诛魔 on 2019/7/26.
//

#include "player_audio_video.h"


static struct SwrContext *p_swr_ctx = NULL;  //音频的上下文
static int quit;

/**
 * 封装解码音频数据
 * @param pContext 上下文
 * @param buffer  容器
 * @param i  大小
 * @return
 */
int audio_decode_frame(AVCodecContext *p_codec_ctx, uint8_t *audio_buffer, size_t buffer_size) {

    static AVPacket av_packet;
    static uint8_t *audio_package_data = NULL;
    static int audio_package_size = 0;
    static AVFrame av_frame;

    int decode_length;
    int data_size;


    //这样,while第一次不会被执行到
    //取到数据则可以for循环,开始while解码数据,取不到数据就 quit
    for (;;) {
        //只要队列里不为空,就一直解码
        while (audio_package_size > 0) {
            int got_frame = 0;
            /*
             * @param      avctx the codec context
             * @param[out] frame The AVFrame in which to store decoded audio samples
             * @param[out] got_frame_ptr Zero if no frame could be decoded, otherwise it is non-zero.
             * @param[in]  avpkt The input AVPacket containing the input buffer.
             * @return number of bytes consumed from the input AVPacket
             */
            decode_length = avcodec_decode_audio4(p_codec_ctx, &av_frame, &got_frame, &av_packet);
            if (decode_length < 0) {
                audio_package_size = 0;
                break;
            }
            audio_package_size -= decode_length;   //剩余长度
            audio_package_data += decode_length;   //已使用长度

            data_size = 0;
            if (got_frame) {
                //解码成功的数据.
                data_size = 2 * 2 * av_frame.nb_samples; //???
                assert(data_size <= buffer_size);
                //convert audio buffer 为标准输入的数据.
                swr_convert(p_swr_ctx,
                            &audio_buffer,
                            MAX_AUDIO_FRAME_SIZE * 3 / 2,
                            (const uint8_t) av_frame.data,
                            av_frame.nb_samples
                );
            }
            if (data_size <= 0) {
                continue;
            }
            return data_size;
        }
        if (av_packet.data) {
            av_free_packet(&av_packet);
        }
        if (quit) {
            return -1;
        }

        //继续取数据
        if (select_audio_packet_queue(NULL, &av_packet, 1) < 0) {
            //取不到数据,则退出
            return -1;
        }

        //赋值
        audio_package_data = av_packet.data;
        audio_package_size = av_packet.size;
    }
}

/**
 * 播放设备的回调,调取数据
 * @param p_audio_ctx  AVCodecContext
 * @param stream 音频设备的buffer
 * @param length buffer可用的大小
 */
void audio_callback(AVCodecContext *p_audio_ctx, Uint8 *stream, int length) {

    AVCodecContext *p_audio_codec_ctx = p_audio_ctx;
    int unused_data_length;  //还没用使用的audio_buffer数据长度
    int decode_audio_size;    //解码过的音频数据长度.

    static uint8_t audio_buffer[(MAX_AUDIO_FRAME_SIZE * 3) / 12];
    //实际上的audio_buffer长度,decode_audio_size可能为空,则填充buffer的就是静默音
    static unsigned int audio_buffer_size = 0;
    //当前已经被使用过的index
    static unsigned int audio_buffer_index = 0;

    //如果大于0,则可以读音频数据
    while (length > 0) {
        //audio_buffer_index >= audio_buffer_size  说明已经读取完所有的buffer数据了
        if (audio_buffer_index >= audio_buffer_size) {
            decode_audio_size = audio_decode_frame(p_audio_codec_ctx, audio_buffer, sizeof(audio_buffer));
            //如果解码成功.
            if (decode_audio_size >= 0) {
                audio_buffer_size = decode_audio_size;
            } else {
                //如果为空,就补上静默音
                audio_buffer_size = 1024;
                memset(audio_buffer, 0, audio_buffer_size);
            }
            //读取到数据,则应该把已使用数据的index置为 0
            audio_buffer_index = 0;
        }
        //计算整个可用解码数据的长度
        unused_data_length = audio_buffer_size - audio_buffer_index;
        if (unused_data_length > length) {
            unused_data_length = length;
        }
        //copy audio buffer
        /*
         * dest  要写入的对象
         * data  写入数据
         * length  写入数据长度
         */
        memcpy(stream, audio_buffer + audio_buffer_index, unused_data_length);
        length = length - unused_data_length;  //剩余容量 = 总容量 - 已使用长度
        stream += unused_data_length;   //写入的对象需要先前移动
        audio_buffer_index += unused_data_length;   //使用过的index
    }

}


/**
 * 使用FFmpeg解码 -> SDL2进行播放
 * @param video_path
 * @return result
 */
int play_audio_video(char *video_path) {
    int result = -1;

    //-------ffmpeg-------
    AVFormatContext *p_format_ctx = NULL;   //解码文件,opening multi-media file
    AVCodecContext *p_codec_ctx_origin = NULL;  //codec context
    AVCodecContext *p_codec_ctx = NULL;  //codec context - copy from [p_codec_ctx_origin]
    AVCodec *p_codec = NULL;  //codecer 解码器
    //audio
    AVCodecContext *p_audio_codec_ctx_origin = NULL;
    AVCodecContext *p_audio_codec_ctx = NULL;
    AVCodec *p_audio_codec = NULL;

    AVFrame *p_frame = NULL;  //用于存放解码后的数据帧
    AVPacket packet;   //文件解码前的数据包
    AVPicture *p_picture_yuv = NULL;  //存放解码后的YUV数据

    struct SwsContext *p_sws_ctx = NULL;  //视屏裁剪的上下文


    float aspect_ratio;  //视屏的比例
    int video_stream;  //video的序号ID
    int audio_stream;  //audio的序号ID
    int read_frame_state;


    //------sdl2--------
    SDL_Window *p_windows = NULL; //sdl创建的windows
    SDL_Renderer *p_renderer = NULL;  //sdl渲染用的render
    SDL_Texture *p_texture = NULL;  //texture

    int window_width = 640, window_height = 480;   //windows的宽高,等会用于调整窗口的大小
    SDL_Rect window_rect;   //窗口的显示范围
    Uint32 pixel_format;   //YUV的编码格式

    //------audio-------
    SDL_Event sdl_event;
    SDL_AudioSpec src_spec;
    SDL_AudioSpec dts_spec;

    //0.判断参数
    if (!video_path) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IllegalArgumentException file_path = %s. \n", video_path);
        return result;
    }

    //1.初始化
    //SDL初始化
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        //创建失败
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to init SDL - %s \n", SDL_GetError());
        return result;
    }
    //FFmpeg的各种解码器
    av_register_all();

    //2.open video file,find video info
    if (avformat_open_input(&p_format_ctx, video_path, NULL, NULL) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to Open Video File %s \n", video_path);
        goto __FAIL;
    }
    //read video stream info
    if (avformat_find_stream_info(p_format_ctx, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed find video stream info.");
        goto __FAIL;
    }
    //dump video/audio
    av_dump_format(p_format_ctx, 0, video_path, 0);
    SDL_Log("call av_dump_format end. \n");

    video_stream = -1;
    audio_stream = -1;
    //find video/audio stream index
    for (int i = 0; i < p_format_ctx->nb_streams; i++) {
        //AVMediaType -> AVMEDIA_TYPE_VIDEO
        if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && video_stream < 0) {
            video_stream = i;
        } else if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream < 0) {
            audio_stream = i;
        }
    }

    SDL_Log("find video stream  = %d. \n", video_stream);
    SDL_Log("find audio stream  = %d. \n", audio_stream);
    if (video_stream == -1 || audio_stream == -1) {
        SDL_Log("Failed find video/audio stream info.");
        goto __FAIL;
    }


    //3.get audio/video stream / codec
    //audio codec
    p_audio_codec_ctx_origin = p_format_ctx->streams[audio_stream]->codec;
    p_audio_codec = avcodec_find_decoder(p_audio_codec_ctx_origin->codec_id);
    SDL_Log("get audio codec context succeed. \n");

    //copy origin audio_codec_ctx
    p_audio_codec_ctx = avcodec_alloc_context3(p_audio_codec);
    if (avcodec_copy_context(p_audio_codec_ctx, p_audio_codec_ctx_origin) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy audio codec context");
        goto __FAIL;
    }

    //set audio settings.
    dts_spec.freq = p_audio_codec_ctx->sample_rate;
    dts_spec.channels = p_audio_codec_ctx->channels;
    dts_spec.format = AUDIO_S16SYS;
    dts_spec.silence = 0;
    dts_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    dts_spec.callback = audio_callback;
    dts_spec.userdata = p_audio_codec_ctx;

    //open audio device
    if (SDL_OpenAudio(&dts_spec, &src_spec)) {
        //0  成功
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open audio device \n");
        goto __FAIL;
    }

    //open audio code
    avcodec_open2(p_audio_codec_ctx, p_audio_codec, NULL);


    //video codec
    p_codec_ctx_origin = p_format_ctx->streams[video_stream]->codec;   //获取到了video的编解码上下文
    SDL_Log("get video codec context succeed. \n");

    //获取解码器.
    p_codec = avcodec_find_decoder(p_codec_ctx_origin->codec_id);
    SDL_Log("get video codec succeed. \n");
    if (p_codec == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed Get video Decoder Context.");
        goto __FAIL;
    }

    //copy origin codec_ctx
    p_codec_ctx = avcodec_alloc_context3(p_codec);
    SDL_Log("copy video codec context succeed. \n");
    if (avcodec_copy_context(p_codec_ctx, p_codec_ctx_origin) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy video codec context");
        goto __FAIL;// Error copying codec context
    }


    //get codec instance
    if (avcodec_open2(p_codec_ctx, p_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed Open Decoder.");
        goto __FAIL;
    }
    SDL_Log("call avcodec_open2,get codec succeed. \n");

    //alloc frame , 解码后的数据帧
    p_frame = av_frame_alloc();

    p_swr_ctx = swr_alloc();
    if (!p_swr_ctx) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed Alloc SwrContext.");
        goto __FAIL;
    }
    //Out Audio Param
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    int64_t in_channel_layout = av_get_default_channel_layout(p_audio_codec_ctx->channels);
    swr_alloc_set_opts(
            p_swr_ctx,
            out_channels,
            AV_SAMPLE_FMT_S16,
            p_audio_codec_ctx->sample_rate,
            in_channel_layout,
            p_audio_codec_ctx->sample_fmt,
            p_audio_codec_ctx->sample_rate,
            0,
            NULL);
    swr_init(p_swr_ctx);


    //4.create SDL windows /render / texture
    //get video size
    window_height = p_codec_ctx->height;
    window_width = p_codec_ctx->width;
    SDL_Log("windows_h = %d , windows_w = %d", window_height, window_width);

    //create windows
    p_windows = SDL_CreateWindow(
            "WangZhuMo's Player",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_width,
            window_height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    SDL_Log("End call SDL_CreateWindow \n");
    if (p_windows == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to Create SDL_Window.");
        goto __FAIL;
    }

    //create Renderer
    p_renderer = SDL_CreateRenderer(p_windows, -1, 0);
    if (p_renderer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to Create renderer.");
        goto __FAIL;
    }
    SDL_Log("End call SDL_CreateRenderer \n");
    //create texture
    pixel_format = SDL_PIXELFORMAT_IYUV;   //通过IYUV的格式显示

    p_texture = SDL_CreateTexture(
            p_renderer,
            pixel_format,
            SDL_TEXTUREACCESS_STREAMING,
            window_width, window_height);
    SDL_Log("End call SDL_CreateTexture \n");
    //5.create SWS context
    p_sws_ctx = sws_getContext(
            p_codec_ctx->width,   //原始宽，高
            p_codec_ctx->height,
            p_codec_ctx->pix_fmt,
            p_codec_ctx->width,   //输出的宽，高
            p_codec_ctx->height,
            AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
    );
    SDL_Log("End call sws_getContext \n");
    //6.read package / refresh yuv data
    //alloc picture
    p_picture_yuv = (AVPicture *) malloc(sizeof(AVPicture));
    avpicture_alloc(
            p_picture_yuv,
            AV_PIX_FMT_YUV420P,
            p_codec_ctx->width,
            p_codec_ctx->height
    );
    SDL_Log("End call avpicture_alloc \n");

    while (av_read_frame(p_format_ctx, &packet) >= 0) {     //read package 读取解码前的数据
        //confirm package from video frame
        if (packet.stream_index == video_stream) {
            //Decode video frame
            avcodec_decode_video2(p_codec_ctx, p_frame, &read_frame_state, &packet);
            //get video frame ?
            if (read_frame_state) {
                //convert image into YUV format
                sws_scale(p_sws_ctx,
                          (uint8_t const *const *) p_frame->data,
                          p_frame->linesize,
                          0,
                          p_codec_ctx->height,
                          p_picture_yuv->data,
                          p_picture_yuv->linesize
                );

                //更新数据到 texture 上
                SDL_UpdateYUVTexture(
                        p_texture,
                        NULL,
                        p_picture_yuv->data[0], p_picture_yuv->linesize[0],
                        p_picture_yuv->data[1], p_picture_yuv->linesize[1],
                        p_picture_yuv->data[2], p_picture_yuv->linesize[2]
                );

                //change windows size
                window_rect.x = 0;
                window_rect.y = 0;
                window_rect.w = p_codec_ctx->width;
                window_rect.h = p_codec_ctx->height;

                //refresh yuv data to windows
                SDL_RenderClear(p_renderer);
                SDL_RenderCopy(p_renderer, p_texture, NULL, &window_rect);
                SDL_RenderPresent(p_renderer);

                av_free_packet(&packet);

            } else if (packet.stream_index == audio_stream) {
                //输入队列中.
                insert_audio_packet_queue(&packet, &packet);
            } else {
                av_free_packet(&packet);
            }

            SDL_Event event;
            SDL_PollEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    goto __QUIT;
                    break;
                default:
                    break;
            }
        }


        __QUIT:
        read_frame_state = 0;  //这样会退出while循环

        __FAIL:
        //free frame data
        if (p_frame) {
            av_frame_free(&p_frame);
        }

        //free pict
        if (p_picture_yuv) {
            avpicture_free(p_picture_yuv);
            free(p_picture_yuv);
        }

        //close codec
        if (p_codec_ctx_origin) {
            avcodec_close(p_codec_ctx_origin);
        }
        if (p_codec_ctx) {
            avcodec_close(p_codec_ctx);
        }

        //close file
        if (p_format_ctx) {
            avformat_close_input(&p_format_ctx);
        }

        //destroy sdl
        if (p_windows) {
            SDL_DestroyWindow(p_windows);
        }

        if (p_renderer) {
            SDL_DestroyRenderer(p_renderer);
        }

        if (p_texture) {
            SDL_DestroyTexture(p_texture);
        }

        SDL_Quit();
        return result;
    }
}

