//
// Created by 王诛魔 on 2019/7/26.
//
#include "player_only_video.h"

int play_only_video(char *video_path) {
    int result = -1;

    //-------ffmpeg-------
    AVFormatContext *p_format_ctx = NULL;   //解码文件,opening multi-media file
    const AVCodecContext *p_codec_ctx_origin = NULL;  //codec context
    AVCodecContext *p_codec_ctx = NULL;  //codec context - copy from [p_codec_ctx_origin]

    AVCodec *p_codec = NULL;  //codecer 解码器
    AVFrame *p_frame = NULL;  //用于存放解码后的数据帧
    AVPacket packet;   //文件解码前的数据包
    AVPicture *p_picture_yuv = NULL;  //存放解码后的YUV数据
    struct SwsContext *p_sws_ctx = NULL;  //视屏裁剪的上下文
    float aspect_ratio;  //视屏的比例
    int video_stream;  //video的序号ID
    int read_frame_state;


    //------sdl2--------
    SDL_Window *p_windows = NULL; //sdl创建的windows
    SDL_Renderer *p_renderer = NULL;  //sdl渲染用的render
    SDL_Texture *p_texture = NULL;  //texture

    int window_width = 640, window_height = 480;   //windows的宽高,等会用于调整窗口的大小
    SDL_Rect window_rect;   //窗口的显示范围
    Uint32 pixel_format;   //YUV的编码格式

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
    /*
     * AVFormatContext ** 	ps,  指针->指针 ?
     * const char * 	url, 
     * AVInputFormat * 	fmt,
     * AVDictionary ** 	options 
     */
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

    video_stream = -1;
    //find video stream
    for (size_t i = 0; i < p_format_ctx->nb_streams; i++) {
        if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)  //AVMediaType -> AVMEDIA_TYPE_VIDEO
        {
            video_stream = i;
            break;
        }
    }

    if (video_stream == -1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed find video stream info.");
        goto __FAIL;
    }

    //3.get video stream / codecer
    p_codec_ctx_origin = p_format_ctx->streams[video_stream]->codec;   //获取到了video的编解码上下文

    //获取解码器.
    p_codec = avcodec_find_decoder(p_codec_ctx_origin->codec_id);
    if (p_codec == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed Get video Decoder Context.");
        goto __FAIL;
    }

    //copy origin codec_ctx
    p_codec_ctx = avcodec_alloc_context3(p_codec);
    if (avcodec_copy_context(p_codec_ctx, p_codec_ctx_origin) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy codec context");
        goto __FAIL;// Error copying codec context
    }


    //get codec instance
    if (avcodec_open2(p_codec_ctx, p_codec, NULL) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed Open Decoder.");
        goto __FAIL;
    }

    //alloc frame , 解码后的数据帧
    p_frame = av_frame_alloc();

    //4.create SDL windows /render / texture
    //get video size
    window_height = p_codec_ctx->height;
    window_width = p_codec_ctx->width;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "windows_h = %d , windows_w = %d", window_height, window_width);

    //create windows
    p_windows = SDL_CreateWindow(
            "WangZhuMo's Player",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_width,
            window_height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

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

    //create texture
    pixel_format = SDL_PIXELFORMAT_IYUV;   //通过IYUV的格式显示
    p_texture = SDL_CreateTexture(
            p_renderer,
            pixel_format,
            SDL_TEXTUREACCESS_STREAMING,
            window_width, window_height);

    //5.create SWS context
    /*
     * @param srcW the width of the source image
     * @param srcH the height of the source image
     * @param srcFormat the source image format
     * input video info
     *
     * @param dstW the width of the destination image
     * @param dstH the height of the destination image
     * @param dstFormat the destination image format
     * target video info
     *
     * @param flags specify which algorithm and options to use for rescaling
     * @param param extra parameters to tune the used scale
     */
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

    //6.read package / refresh yuv data
    //New Api
//    int av_codec_result;
//    av_codec_result = avcodec_send_packet(p_codec_ctx, &packet);
//    if (av_codec_result < 0) {
//        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "avcodec_send_frame failed.");
//        goto __FAIL;
//    }
//    while (av_codec_result >= 0) {
//        av_codec_result = avcodec_receive_frame(p_codec_ctx, p_frame);
//        if (av_codec_result == AVERROR(EAGAIN) || av_codec_result == AVERROR_EOF) {
//            return result;
//        } else if (av_codec_result < 0) {
//            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error during decode video package.");
//            goto __FAIL;
//        }
//    }
    while (av_read_frame(p_format_ctx, &packet) > 0) {     //read package 读取解码前的数据
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
            }
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
        avformat_close_input(p_format_ctx);
    }

    //destroy sdl
    if (p_windows) {
        SDL_DestroyWindow(p_windows);
    }
    if (p_texture) {
        SDL_DestroyTexture(p_texture);
    }
    if (p_renderer) {
        SDL_DestroyRenderer(p_renderer);
    }
    SDL_Quit();

    return result;
}