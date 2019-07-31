//
// Created by 王诛魔 on 2019/8/1.
//

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

/**
 * 给队列中存入数据
 * @param q
 * @param pkt
 * @return
 */
int insert_audio_packet_queue(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt1;
    int ret;
    SDL_LockMutex(q->mutex);
    for (;;) {
        if (global_video_state->quit) {
            fprintf(stderr, "quit from queue_get\n");
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            fprintf(stderr, "queue is empty, so wait a moment and wait a cond signal\n");
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}


/**
 * 获取队列中的数据
 * @param q
 * @param pkt
 * @param block
 * @return
 */
int select_audio_packet_queue(PacketQueue *q, AVPacket *pkt,int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for(;;) {

        if(global_video_state->quit) {
            fprintf(stderr, "quit from queue_get\n");
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            fprintf(stderr, "queue is empty, so wait a moment and wait a cond signal\n");
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}