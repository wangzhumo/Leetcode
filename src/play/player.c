//
// Created by 王诛魔 on 2019/7/26.
//
#include "player.h"



int start_play_video(char *video_path){
    int result = -1;

    //-------ffmpeg-------
    AVFormatContext *p_format_ctx = NULL;   //解开文件,opening multi-media file
    AVCodecContext *p_codec_ctx_origin = NULL;  //codec context

    AVCodec *p_codec = NULL;  //codecer 解码器
    AVFrame *p_frame = NULL;  //用于存放解码后的数据帧
    AVPacket packet;   //文件解码前的数据包
    AVPicture *p_picture_yuv = NULL;  //存放解码后的YUV数据
    struct SwsContext *p_sws_ctx = NULL;  //视屏裁剪的上下文
    float aspect_ratio;  //视屏的比例
    int vidoe_stream;  //video的序号ID


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


    if (avformat_find_stream_info(p_format_ctx,NULL) <0){
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed find stream info.");
        goto __FAIL;
    }

__FAIL:
    SDL_Quit();
    return result;
}