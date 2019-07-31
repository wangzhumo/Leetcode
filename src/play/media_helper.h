//
// Created by 王诛魔 on 2019/7/29.
//
#ifndef MEDIA_MEDIA_HELPER_H
#define MEDIA_MEDIA_HELPER_H

#endif //MEDIA_MEDIA_HELPER_H

#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>



typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;




typedef struct VideoState {
    //for multi-media file
    char filename[1024];
    AVFormatContext *pFormatCtx;

    int videoStream, audioStream;

    //for audio
    AVStream *audio_st;
    AVCodecContext *audio_ctx;
    PacketQueue audioq;
    uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
    AVFrame audio_frame;
    AVPacket audio_pkt;
    uint8_t *audio_pkt_data;
    int audio_pkt_size;
    struct SwrContext *audio_swr_ctx;

    //for video
    AVStream *video_st;
    AVCodecContext *video_ctx;
    PacketQueue videoq;
    struct SwsContext *sws_ctx;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
    int pictq_size, pictq_rindex, pictq_windex;

    //for thread
    SDL_mutex *pictq_mutex;
    SDL_cond *pictq_cond;

    SDL_Thread *parse_tid;
    SDL_Thread *video_tid;

    int quit;

} VideoState;

/**
 * 获取队列中的数据
 * @param q
 * @param pkt
 * @return
 */
int select_audio_packet_queue(PacketQueue *q, AVPacket *pkt, int block);

/**
 * 给队列中存入数据
 * @param q
 * @param pkt
 * @param block
 * @return
 */
int insert_audio_packet_queue(PacketQueue *q, AVPacket *pkt);

void packet_queue_init(PacketQueue *q);

struct VideoState *global_video_state;