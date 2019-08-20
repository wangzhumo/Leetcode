//
// Created by 王诛魔 on 2019/7/29.
//
#ifndef MEDIA_MEDIA_HELPER_H
#define MEDIA_MEDIA_HELPER_H

#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avstring.h>
#include <libswresample/swresample.h>

#endif //MEDIA_MEDIA_HELPER_H



#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)

#define VIDEO_PICTURE_QUEUE_SIZE 1

SDL_mutex    *p_texture_mutex;
SDL_Window   *p_sdl_windows = NULL;
SDL_Renderer *p_sdl_renderer;
SDL_Texture  *p_sdl_texture;


typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;   //FFmpeg提供，单链表，其中 next -> 指向下一个元素
    int nb_packets;     //packet 的数量
    int size;           //size 总的大小
    SDL_mutex *mutex;   //互斥，用于加锁
    SDL_cond *cond;     //处理同步使用
} PacketQueue;

typedef struct VideoPicture {
    AVPicture *picture;
    int width, height; /* source height & width */
    int allocated;
    double pts;
} VideoPicture;

/**
 * 保存所有的媒体信息.
 */
typedef struct VideoState {
    //for multi-media file
    char filename[1024];                    //媒体文件
    AVFormatContext *pFormatCtx;            //媒体的上下文 - 打开媒体文件后

    int videoIndex, audioIndex;             //音/视频的index

    //for audio
    AVStream *audio_stream;                 //音频流
    AVCodecContext *audio_ctx;              //音频解码上下文
    PacketQueue audio_queue;                //音频解码队列
    uint8_t audio_buffer[(MAX_AUDIO_FRAME_SIZE * 3) / 2];  //解码后的音频buffer
    unsigned int audio_buffer_size;         //解码的长度
    unsigned int audio_buffer_index;        //解码后buffer的使用的长度  [audio_buffer_index,audio_buffer_size]就是未使用的长度
    AVFrame audio_frame;                    //音频帧
    AVPacket audio_packet;                  //解码前音频packet
    uint8_t *audio_packet_data;             //解码前音频packet -> 数据的指针
    int audio_packet_size;                  //数据长度
    struct SwrContext *audio_swr_ctx;       //音频转换器的上下文,重采样

    //for video
    AVStream *video_stream;                 //视频流
    AVCodecContext *video_ctx;              //视频解码上下文
    PacketQueue video_queue;                //视频待解码队列
    struct SwsContext *video_sws_ctx;       //视频重采样上下文

    VideoPicture picture_queue[VIDEO_PICTURE_QUEUE_SIZE];       //视频的每一帧(YUV),已经解码完毕
    int picture_queue_size;                 //视频队列的长度
    int picture_queue_rindex;               //视频队列的读取位置
    int picture_queue_windex;               //视频队列的写入位置

    //for thread
    SDL_mutex *picture_queue_mutex;         //视频帧队列的锁
    SDL_cond *picture_queue_cond;           //视频帧队列的信号量

    SDL_Thread *parse_demux_tid;            //解复用线程
    SDL_Thread *video_decode_tid;           //视频解码线程

    int quit;                               //结束进程.

} VideoState;


void packet_queue_init(PacketQueue *q);

int select_packet_queue(PacketQueue *p_packet_queue, AVPacket *p_out_pkt, int block);

int insert_packet_queue(PacketQueue *p_packet_queue, AVPacket *in_pkt);

int open_audio_devices(AVCodecContext *p_codec_ctx,
                       SDL_AudioSpec *wanted_spec,
                       SDL_AudioSpec *spec,
                       void *user_data,
                       SDL_AudioCallback callback);

void alloc_picture(void *userdata);

int queue_picture(VideoState *is, AVFrame *pFrame, double pts);

static VideoState global_video_state;