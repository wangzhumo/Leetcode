//
// Created by 王诛魔 on 2019/7/26.
//
#include "player.h"

int start_play_video(char *video_path){
    int result = -1;

    //-------ffmpeg-------
    AVFormatContext *p_format_ctx = NULL;   //解开文件,opening multi-media file
    const AVCodecContext *p_codec_ctx_origin = NULL;  //codec context
    AVCodecContext *p_codec_ctx = NULL;  //codec context - copy from [p_codec_ctx_origin]

    AVCodec *p_codec = NULL;  //codecer 解码器
    AVFrame *p_frame = NULL;  //用于存放解码后的数据帧
    AVPacket packet;   //文件解码前的数据包
    AVPicture *p_picture_yuv = NULL;  //存放解码后的YUV数据
    struct SwsContext *p_sws_ctx = NULL;  //视屏裁剪的上下文
    float aspect_ratio;  //视屏的比例
    int video_stream;  //video的序号ID


    //------sdl2--------
    SDL_Window *p_windows = NULL; //sdl创建的windows
    SDL_Renderer *p_renderer = NULL;  //sdl渲染用的render
    SDL_Texture *p_texture = NULL;  //texture

    int window_width = 640, window_height = 480;   //windows的宽高,等会用于调整窗口的大小
    SDL_Rect window_rect;   //窗口的显示范围
    Uint32 pixel_format;   //YUV的编码格式

    //0.判断参数
    if (video_path){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"IllegalArgumentException file_path. \n");
        return result; 
    }
    

    //1.初始化
    //SDL初始化
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
        //创建失败
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed to init SDL - %s \n" , SDL_GetError());
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
    if (avformat_open_input(&p_format_ctx,video_path,NULL,NULL)!=0){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed to Open Video File %s \n" , video_path);
        goto __FAIL;
    }
    //read video stream info
    if (avformat_find_stream_info(p_format_ctx,NULL) <0){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed find video stream info.");
        goto __FAIL;
    }
    //dump video/audio
    av_dump_format(p_format_ctx,0,video_path,0);

    video_stream = -1;
    //find video stream
    for (size_t i = 0; i < p_format_ctx->nb_streams; i++)
    {   
        if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)  //AVMediaType -> AVMEDIA_TYPE_VIDEO
        {
            video_stream = i; 
            break;
        }
    }

    if (video_stream == -1)
    {
       SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed find video stream info."); 
       goto __FAIL;
    }

    //3.get video stream / codecer
    p_codec_ctx_origin = p_format_ctx->streams[video_stream]->codec;   //获取到了video的编解码上下文
    //获取解码器.
    p_codec = avcodec_find_decoder(p_codec_ctx_origin->codec_id);
    if (p_codec == NULL)
    {
       SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed Get video Decoder Context."); 
       goto __FAIL;
    }
    //copy origin codec_ctx
    p_codec_ctx = avcodec_alloc_context3(p_codec);
    //get codec instance
    if (avcodec_open2(p_codec_ctx,p_codec,NULL) < 0){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed Get video Decoder."); 
        goto __FAIL; 
    }


__FAIL:
    SDL_Quit();
    return result;
}