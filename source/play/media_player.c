//
// Created by 王诛魔 on 2019/8/1.
//

#include <assert.h>
#include "media_player.h"

static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque) {
    SDL_Event event;
    event.type = FF_REFRESH_EVENT;
    event.user.data1 = opaque;
    SDL_PushEvent(&event);
    return 0; /* 0 means stop timer */
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(VideoState *is, Uint32 delay) {
    SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
}

/**
 * 封装解码音频数据
 * @param pContext 上下文
 * @param buffer  容器
 * @param i  大小
 * @return
 */
int audio_decode_frame(VideoState *state, uint8_t *audio_buffer, size_t buffer_size, double *pts_ptr) {

    AVPacket *p_av_packet = &state->audio_packet;

    static uint8_t *audio_package_data = NULL;

    int decode_length;   //解码的长度
    int data_size;
    double pts;
    int n;


    //这样,while第一次不会被执行到
    //取到数据则可以for循环,开始while解码数据,取不到数据就 quit
    for (;;) {
        //只要队列里不为空,就一直解码
        while (state->audio_packet_size > 0) {
            int got_frame = 0;
            /*
             * @param      avctx the codec context
             * @param[out] frame The AVFrame in which to store decoded audio samples
             * @param[out] got_frame_ptr Zero if no frame could be decoded, otherwise it is non-zero.
             * @param[in]  avpkt The input AVPacket containing the input buffer.
             * @return number of bytes consumed from the input AVPacket
             */
            decode_length = avcodec_decode_audio4(state->audio_ctx, &state->audio_frame, &got_frame, p_av_packet);
            if (decode_length < 0) {
                state->audio_packet_size = 0;
                break;
            }

            data_size = 0;
            if (got_frame) {
                //解码成功的数据.
                data_size = 2 * 2 * state->audio_frame.nb_samples;
                assert(data_size <= buffer_size);
                //convert audio buffer 为标准输入的数据.
                swr_convert(state->audio_swr_ctx,
                            &audio_buffer,
                            MAX_AUDIO_FRAME_SIZE * 3 / 2,
                            (const uint8_t **) state->audio_frame.data,
                            state->audio_frame.nb_samples
                );
                //fwrite(audio_buffer, 1, data_size, audiofd);
            }

            state->audio_packet_size -= decode_length;   //剩余大小
            state->audio_packet_data += decode_length;   //加上解码后的长度,指针到了没有使用过的数据位置


            if (data_size <= 0) {
                continue;
            }

            //此时.
            pts = state->audio_clock;
            *pts_ptr = pts;

            n = 2 * state->audio_ctx->channels;
            state->audio_clock += data_size / (double) (n * state->audio_ctx->sample_rate);
            return data_size;
        }

        if (p_av_packet->data) {
            av_free_packet(p_av_packet);
        }
        if (state->quit) {
            return -1;
        }

        //继续取数据
        if (select_packet_queue(&state->audio_queue, p_av_packet, 1) < 0) {
            //取不到数据,则退出
            return -1;
        }

        //赋值
        state->audio_packet_data = p_av_packet->data;
        state->audio_packet_size = p_av_packet->size;

        /* if update, update the audio clock w/pts */
        if (p_av_packet->pts != AV_NOPTS_VALUE) {
            //Convert an AVRational to a `double`.
            state->audio_clock = av_q2d(state->audio_stream->time_base) * p_av_packet->pts;
        }
    }
}


/**
 * 播放设备的回调,调取数据
 * @param p_audio_ctx  AVCodecContext
 * @param stream 音频设备的buffer
 * @param length buffer可用的大小
 */
void audio_callback_func(void *p_audio_ctx, Uint8 *stream, int length) {
    VideoState *state = (VideoState *) p_audio_ctx;
    int unused_data_length;  //还没用使用的audio_buffer数据长度
    int decode_audio_size;    //解码过的音频数据长度.
    double pts;

    SDL_memset(stream, 0, length);

    //如果大于0,则可以读音频数据
    while (length > 0) {
        //audio_buffer_index >= audio_buffer_size  说明已经读取完所有的buffer数据了
        if (state->audio_buffer_index >= state->audio_buffer_size) {
            //继续解码音频数据
            decode_audio_size = audio_decode_frame(state, state->audio_buffer, sizeof(state->audio_buffer), &pts);
        }
    }
}

/**
 * 视频解码线程
 * @param args
 * @return int
 */
int decode_video_thread(void *args) {

    VideoState *state = (VideoState *) args;
    AVPacket pkg1, *p_packet = &pkg1;
    int frameFinished;
    AVFrame *p_frame = NULL;

    //开辟一个frame
    p_frame = av_frame_alloc();

    //获取视屏解封装的packet
    for (;;) {
        //get packet from video_queue
        if (select_packet_queue(&state->video_queue, p_packet, 1) < 0) {
            //不需要继续解码了
            break;
        }
        //否则,开始解码 p_packet 中的数据
        /*
         * @param         codec context
         * @param[out]    picture The AVFrame in which the decoded video frame will be stored.
         * @param[in,out] got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero.
         */
        avcodec_decode_video2(state->video_ctx, p_frame, &frameFinished, p_packet);

        //got_picture_ptr 为 0,说明没有要解码的数据了.
        if (frameFinished) {
            //有解码后的数据,则放到queue_picture中去
            if (queue_picture(state, p_frame, 0) < 0) {
                break;
            }
        }
        av_free_packet(p_packet);
    }
    av_frame_free(&p_frame);
    return 0;
}

/**
 * 打开指定的媒体流
 * @param state
 * @param stream_index
 * @return int 表示是否产生错误.
 */
int stream_component_open(VideoState *state, int stream_index) {

    AVFormatContext *p_format_ctx = state->pFormatCtx;
    AVCodecContext *p_codec_ctx = NULL;
    AVCodec *p_codec = NULL;
    SDL_AudioSpec wanted_spec, spec;
    int64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int64_t in_channel_layout = 0;

    //stream_index是否可用
    if (stream_index < 0 || stream_index > p_format_ctx->nb_streams) {
        SDL_Log("stream index can't open.\n");
        return -1;
    }

    //1.get stream codec
    p_codec = avcodec_find_decoder(p_format_ctx->streams[stream_index]->codec->codec_id);
    if (!p_codec) {
        SDL_Log("avcodec_find_decoder fail\n");
        fprintf(stderr, "unsupport decoder.\n");
        return -1;
    }

    //2.copy codec_context
    p_codec_ctx = avcodec_alloc_context3(p_codec);
    if (avcodec_copy_context(p_codec_ctx, p_format_ctx->streams[stream_index]->codec) != 0) {
        fprintf(stderr, "avcodec_copy_context codecContext fail.\n");
        return -1;
    }

    //3.open codec
    if (avcodec_open2(p_codec_ctx, p_codec, NULL) < 0) {
        fprintf(stderr, "unsupport codec. \n");
    }

    if (p_codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        //open audio devices
        open_audio_devices(p_codec_ctx, &wanted_spec, &spec, state, audio_callback_func);
        //set video_state
        state->audioIndex = stream_index;
        state->audio_ctx = p_codec_ctx;
        state->audio_stream = p_format_ctx->streams[stream_index];
        state->audio_buffer_size = 0;
        state->audio_buffer_index = 0;

        //get channel layout
        in_channel_layout = av_get_default_channel_layout(p_codec_ctx->channels);

        //开辟空间
        memset(&state->audio_packet, 0, sizeof(state->audio_packet));
        packet_queue_init(&state->audio_queue);


        //创建重采样.
        state->audio_swr_ctx = swr_alloc();
        swr_alloc_set_opts(
                state->audio_swr_ctx,
                out_channel_layout,
                AV_SAMPLE_FMT_S16,
                state->audio_ctx->sample_rate,
                in_channel_layout,
                state->audio_ctx->sample_fmt,
                state->audio_ctx->sample_rate,
                0,
                NULL
        );

        fprintf(stderr,
                "swr opts: out_channel_layout:%lld, out_sample_fmt:%d, out_sample_rate:%d, in_channel_layout:%lld, in_sample_fmt:%d, in_sample_rate:%d",
                out_channel_layout, AV_SAMPLE_FMT_S16, state->audio_ctx->sample_rate, in_channel_layout,
                state->audio_ctx->sample_fmt,
                state->audio_ctx->sample_rate);

        swr_init(state->audio_swr_ctx);

        SDL_PauseAudio(0);  //开始播放
    } else if (p_codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        state->videoIndex = stream_index;
        state->video_ctx = p_codec_ctx;
        state->video_stream = p_format_ctx->streams[stream_index];
        //创建视频的queue
        packet_queue_init(&state->video_queue);
        //创建视频解码线程.
        state->video_decode_tid = SDL_CreateThread(
                NULL,
                "video_decode_thread",
                state);
        //视频裁剪.
        state->video_sws_ctx = sws_getContext(
                state->video_ctx->width,
                state->video_ctx->height,
                state->video_ctx->pix_fmt,
                state->video_ctx->width,
                state->video_ctx->height,
                AV_PIX_FMT_YUV420P,
                SWS_BILINEAR,
                NULL,
                NULL,
                NULL
        );
    }
    return 0;
}


/**
 * 解复用线程
 * @param args VideoState
 * @return int 表示是否产生错误.
 */
int demux_thread(void *args) {

    VideoState *videoState = (VideoState *) args;
    AVFormatContext *p_format_ctx = NULL;   //解码文件

    int video_stream = -1;
    int audio_stream = -1;



    //1.open file
    if (avformat_open_input(&p_format_ctx, videoState->filename, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to Open Video File %s \n", videoState->filename);
        return -1;
    }
    videoState->pFormatCtx = p_format_ctx;

    //2.read file info -> dump
    if (avformat_find_stream_info(p_format_ctx, NULL) < 0) {
        fprintf(stderr, "Failed find video stream info.\n");
        return -1;
    }

    av_dump_format(p_format_ctx, 0, videoState->filename, 0);
    SDL_Log("call av_dump_format end. \n");

    //3.find media stream
    for (size_t i = 0; i < p_format_ctx->nb_streams; i++) {
        if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
            video_stream < 0) { //AVMediaType -> AVMEDIA_TYPE_VIDEO{
            //find video stream
            video_stream = i;

        } else if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                   audio_stream < 0) {
            //find audio stream
            audio_stream = i;
        }
    }
    videoState->videoIndex = video_stream;
    videoState->audioIndex = audio_stream;

    //4.open video/audio stream
    stream_component_open(videoState, video_stream);  //set videoState->videoIndex
    stream_component_open(videoState, audio_stream);  //set videoState->audioIndex

    if (videoState->videoIndex == -1 || videoState->audioIndex == -1) {
        fprintf(stderr, "Failed find video/audio stream info.\n");
        goto __ERROR;
    }
    SDL_Log("find video stream succeed. \n");


    __ERROR:
    if (p_format_ctx) {
        avformat_close_input(&p_format_ctx);
    }

    return 0;
}


int play(char *path) {

    SDL_Event event;
    VideoState *state = NULL;
    Uint32 pixel_format;   //YUV的编码格式
    AVPacket packet, *p_packet = &packet;

    //**init VideoState
    state = av_malloc(sizeof(VideoState));
    //copy file path
    av_strlcpy(state->filename, path, sizeof(state->filename));
    //register all
    av_register_all();

    //由于MACOS必须在主线程创建 SDL_window
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    //**创建线程锁，创建信号量
    state->picture_queue_mutex = SDL_CreateMutex();
    state->picture_queue_cond = SDL_CreateCond();

    //**create timer,每40毫秒发送一次
    // SDL_PushEvent(&event);    state 为参数
    schedule_refresh(state, 40);  //定时去渲染视频信息.

    //**创建一个解复用线程.
    state->parse_demux_tid = SDL_CreateThread(demux_thread, "demux_thread", state);
    if (!state->parse_demux_tid) {
        //创建失败就退出.
        av_free(state);
        goto __FAIL;
    }

    //**create window
    p_sdl_windows = SDL_CreateWindow(
            "Wangzhumo's Player",
            0, 0,
            640, 480,
            SDL_WINDOW_RESIZABLE
    );

    if (!p_sdl_windows) {
        fprintf(stderr, "SDL: could not set video mode:%s - exiting.\n", SDL_GetError());
        exit(1);
    }

    //**create renderer
    p_sdl_renderer = SDL_CreateRenderer(p_sdl_windows, -1, 0);
    if (!p_sdl_renderer) {
        fprintf(stderr, "SDL: could not create renderer.\n");
        exit(1);
    }

    //**create texture
    pixel_format = SDL_PIXELFORMAT_IYUV;
    //创建texture的锁
    p_texture_mutex = SDL_CreateMutex();

    p_sdl_texture = SDL_CreateTexture(
            p_sdl_renderer,
            pixel_format,
            SDL_TEXTUREACCESS_STREAMING,
            state->video_ctx->width,
            state->video_ctx->height);

    for (;;) {
        if (state->quit) {
            SDL_CondSignal(state->video_queue.cond);
            SDL_CondSignal(state->audio_queue.cond);
            break;
        }

        //读取媒体流的frame - packet
        if (av_read_frame(state->pFormatCtx, p_packet) < 0) {
            if (state->pFormatCtx->pb->error == 0) {
                SDL_Delay(100);   //这表示不是错误,需要等待输入
                continue;
            } else {
                break;
            }
        }

        //如果以上都OK,把解封装的packet放入到queue中去
        if (p_packet->stream_index == state->videoIndex) {
            //音频数据
            insert_packet_queue(&state->video_queue, p_packet);
            fprintf(stderr, "insert_packet_queue,video size = %d \n", state->video_queue.nb_packets);
        } else if (p_packet->stream_index == state->audioIndex) {
            //音频数据
            insert_packet_queue(&state->audio_queue, p_packet);
            fprintf(stderr, "insert_packet_queue,audio size = %d \n", state->audio_queue.nb_packets);
        } else {
            av_packet_free(p_packet);
        }
    }

    __FAIL:
    SDL_Log("sdl_event quit.");
    SDL_Event event1;
    event1.type = FF_QUIT_EVENT;
    event1.user.data1 = state;
    SDL_PushEvent(&event1);

    //destroy sdl
    if (p_sdl_windows) {
        SDL_DestroyWindow(p_sdl_windows);
    }

    if (p_sdl_renderer) {
        SDL_DestroyRenderer(p_sdl_renderer);
    }

    if (p_sdl_texture) {
        SDL_DestroyTexture(p_sdl_texture);
    }

    SDL_Quit();
}

