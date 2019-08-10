//
// Created by 王诛魔 on 2019/8/1.
//

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
 * 播放设备的回调,调取数据
 * @param p_audio_ctx  AVCodecContext
 * @param stream 音频设备的buffer
 * @param length buffer可用的大小
 */
void audio_callback(void *p_audio_ctx, Uint8 *stream, int length) {

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
        open_audio_devices(p_codec_ctx, &wanted_spec, &spec, state, audio_callback);
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
                out_channel_layout, AV_SAMPLE_FMT_S16, state->audio_ctx->sample_rate, in_channel_layout, state->audio_ctx->sample_fmt,
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

    if (video_stream == -1 || audio_stream == -1) {
        fprintf(stderr, "Failed find video/audio stream info.\n");
        goto __ERROR;
    }
    SDL_Log("find video stream succeed. \n");

    //4.open video/audio stream
    stream_component_open(videoState, video_stream);
    stream_component_open(videoState, audio_stream);


    __ERROR:
    if (p_format_ctx) {
        avformat_close_input(&p_format_ctx);
    }

    return 0;
}


int play(char *path) {

    SDL_Event event;
    VideoState *state = NULL;
    SDL_Window *p_sdl_window;
    SDL_Renderer *p_sdl_renderer;
    SDL_Texture *p_sdl_texture;
    SDL_mutex *p_text_mutex;

    //init VideoState
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

    //create window
    p_sdl_window = SDL_CreateWindow(
            "Wangzhumo's Player",
            0, 0,
            640, 480,
            SDL_WINDOW_RESIZABLE
    );

    if (!p_sdl_window) {
        fprintf(stderr, "SDL: could not set video mode:%s - exiting.\n", SDL_GetError());
        exit(1);
    }

    //create renderer
    p_sdl_renderer = SDL_CreateRenderer(p_sdl_window, -1, 0);
    if (!p_sdl_renderer) {
        fprintf(stderr, "SDL: could not create renderer.\n");
        exit(1);
    }
    //创建texture的锁
    p_text_mutex = SDL_CreateMutex();

    //创建线程锁，创建信号量
    state->picture_queue_mutex = SDL_CreateMutex();
    state->picture_queue_cond = SDL_CreateCond();

    //create timer,每40毫秒发送一次
    // SDL_PushEvent(&event);    state 为参数
    schedule_refresh(state, 40);  //定时去渲染视频信息.

    //创建一个解复用线程.
    state->parse_demux_tid = SDL_CreateThread(NULL, "demux_thread", state);
    if (!state->parse_demux_tid) {
        //创建失败就退出.
        av_free(state);
        goto __FAIL;
    }

    __FAIL:
    //destroy sdl
    if (p_sdl_window) {
        SDL_DestroyWindow(p_sdl_window);
    }

    if (p_sdl_renderer) {
        SDL_DestroyRenderer(p_sdl_renderer);
    }

    if (p_sdl_texture) {
        SDL_DestroyTexture(p_sdl_texture);
    }

    SDL_Quit();
}

