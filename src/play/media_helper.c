//
// Created by 王诛魔 on 2019/8/1.
//

#include <tkDecls.h>
#include "media_helper.h"


/**
 * 初始化
 * @param q
 */
void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

void alloc_picture(void *userdata) {

    int ret;

    VideoState *is = (VideoState *)userdata;
    VideoPicture *vp;

    vp = &is->picture_queue[is->picture_queue_windex];
    if(vp->picture) {

        // we already have one make another, bigger/smaller
        avpicture_free(vp->picture);
        free(vp->picture);

        vp->picture = NULL;
    }

    // Allocate a place to put our YUV image on that screen
    SDL_LockMutex(is->picture_queue_mutex);

    vp->picture = (AVPicture*)malloc(sizeof(AVPicture));
    ret = avpicture_alloc(vp->picture, AV_PIX_FMT_YUV420P, is->video_ctx->width, is->video_ctx->height);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate temporary picture: %s\n", av_err2str(ret));
    }

    SDL_UnlockMutex(text_mutex);

    vp->width = is->video_ctx->width;
    vp->height = is->video_ctx->height;
    vp->allocated = 1;

}

/**
 * picture的队列(已经解码完毕)
 * @param is
 * @param pFrame
 * @param pts
 * @return
 */
int queue_picture(VideoState *is, AVFrame *pFrame, double pts) {

    VideoPicture *vp;

    /* wait until we have space for a new pic */
    SDL_LockMutex(is->picture_queue_mutex);
    while (is->picture_queue_size >= VIDEO_PICTURE_QUEUE_SIZE &&
           !is->quit) {
        SDL_CondWait(is->picture_queue_cond, is->picture_queue_mutex);
    }
    SDL_UnlockMutex(is->picture_queue_mutex);

    if (is->quit)
        return -1;

    // windex is set to 0 initially
    vp = &is->picture_queue[is->picture_queue_windex];

    /* allocate or resize the buffer! */
    if (!vp->picture ||
        vp->width != is->video_ctx->width ||
        vp->height != is->video_ctx->height) {

        vp->allocated = 0;
        alloc_picture(is);
        if (is->quit) {
            return -1;
        }
    }

    /* We have a place to put our picture on the queue */
    if (vp->picture) {

        vp->pts = pts;

        // Convert the image into YUV format that SDL uses
        sws_scale(is->video_sws_ctx, (uint8_t const *const *) pFrame->data,
                  pFrame->linesize, 0, is->video_ctx->height,
                  vp->picture->data, vp->picture->linesize);

        /* now we inform our display thread that we have a pic ready */
        if (++is->picture_queue_windex == VIDEO_PICTURE_QUEUE_SIZE) {
            is->picture_queue_windex = 0;
        }

        SDL_LockMutex(is->picture_queue_mutex);
        is->picture_queue_size++;
        SDL_UnlockMutex(is->picture_queue_mutex);
    }
    return 0;

}

/**
 * 给队列中存入数据
 * @param p_packet_queue
 * @param in_pkt
 * @return
 */
int insert_packet_queue(PacketQueue *p_packet_queue, AVPacket *in_pkt) {
    AVPacketList *pkt;
    if (av_dup_packet(in_pkt) < 0) {        //引用计数 +1
        return -1;
    }
    pkt = av_malloc(sizeof(AVPacketList));  // 为pkt1分配一个空间
    if (!pkt) {
        return -1;
    }
    pkt->pkt = *in_pkt;     //传入的packet赋值给 pkt
    pkt->next = NULL;       //是队列的最后一个,他之后没有元素

    SDL_LockMutex(p_packet_queue->mutex);        //加入锁.

    if (!p_packet_queue->last_pkt) {             //如果队列最后一个元素为空,则把自己置为第一个元素
        p_packet_queue->first_pkt = pkt;
    } else {
        p_packet_queue->last_pkt->next = pkt;    //队列不为空,则把自己放在最后一个元素 -> next 上
    }
    p_packet_queue->last_pkt = pkt;              //计算其他队列信息
    p_packet_queue->nb_packets++;
    p_packet_queue->size += pkt->pkt.size;

    SDL_CondSignal(p_packet_queue->cond);        //告知等待获取元素的线程,可以取数据了.   解锁->发信号->加锁

    SDL_UnlockMutex(p_packet_queue->mutex);      //插入完毕,释放锁.
    return 0;
}


/**
 * 获取队列中的数据
 * @param p_packet_queue
 * @param p_out_pkt
 * @param block
 * @return
 */
int select_packet_queue(PacketQueue *p_packet_queue, AVPacket *p_out_pkt, int block) {
    AVPacketList *temp_packet;
    int ret;

    SDL_LockMutex(p_packet_queue->mutex);   //加锁

    for (;;) {
        if (global_video_state.quit) {
            fprintf(stderr, "quit from queue_get\n");
            ret = -1;
            break;
        }

        temp_packet = p_packet_queue->first_pkt;    //取出队列中第一个元素
        if (temp_packet) {                          //如果取到元素,则要修改队列的信息.
            p_packet_queue->first_pkt = temp_packet->next;   //把第一个元素设置为 temp_packet->next
            if (!p_packet_queue->first_pkt){         //如果  temp_packet->next 为空,则队列为空  last_pkt = NULL
                p_packet_queue->last_pkt = NULL;
            }
            p_packet_queue->nb_packets--;   //设置元素的信息
            p_packet_queue->size -= temp_packet->pkt.size;
            *p_out_pkt = temp_packet->pkt;          //p_out_pkt 赋值
            av_free(temp_packet);                   //temp_packet 使用完毕,则释放
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            fprintf(stderr, "queue is empty, so wait a moment and wait a cond signal\n");
            SDL_CondWait(p_packet_queue->cond, p_packet_queue->mutex);  //等待信号
        }
    }
    SDL_UnlockMutex(p_packet_queue->mutex);         //解🔐
    return ret;
}


/**
 * 打开音频设备.
 * @param p_codec_ctx
 * @param output_spec
 * @param input_spec
 * @return error -1
 */
int open_audio_devices(AVCodecContext *p_codec_ctx,
                       SDL_AudioSpec *wanted_spec,
                       SDL_AudioSpec *spec,
                       void *user_data,
                       SDL_AudioCallback callback){
    if (p_codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
        wanted_spec->freq = p_codec_ctx->sample_rate;
        wanted_spec->format = AUDIO_S16SYS;
        wanted_spec->channels = p_codec_ctx->channels;
        wanted_spec->samples = SDL_AUDIO_BUFFER_SIZE;
        wanted_spec->silence = 0;
        wanted_spec->userdata = user_data;
        wanted_spec->callback = callback;
        //open devices
        if (SDL_OpenAudio(wanted_spec, spec) < 0) {
            fprintf(stderr, "SDL_OpenAudio fail %s. \n", SDL_GetError());
            return -1;
        }
        return 0;
    }
    return -1;
}