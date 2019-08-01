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
 * @param p_packet_queue
 * @param in_pkt
 * @return
 */
int insert_audio_packet_queue(PacketQueue *p_packet_queue, AVPacket *in_pkt) {
    AVPacketList *pkt;
    if (av_dup_packet(in_pkt) < 0) {   //引用计数 +1
        return -1;
    }
    pkt = av_malloc(sizeof(AVPacketList));  // 为pkt1分配一个空间
    if (!pkt) {
        return -1;
    }
    pkt->pkt = *in_pkt;     //传入的packet赋值给 pkt
    pkt->next = NULL;       //是队列的最后一个,他之后没有元素

    SDL_LockMutex(p_packet_queue->mutex);    //加入锁.

    if (!p_packet_queue->last_pkt) {      //如果队列最后一个元素为空,则把自己置为第一个元素
        p_packet_queue->first_pkt = pkt;
    } else {
        p_packet_queue->last_pkt->next = pkt;   //队列不为空,则把自己放在最后一个元素 -> next 上
    }
    p_packet_queue->last_pkt = pkt;    //计算其他队列信息
    p_packet_queue->nb_packets++;
    p_packet_queue->size += pkt->pkt.size;

    SDL_CondSignal(p_packet_queue->cond);   //告知等待获取元素的线程,可以取数据了.   解锁->发信号->加锁

    SDL_UnlockMutex(p_packet_queue->mutex);  //插入完毕,释放锁.
    return 0;
}


/**
 * 获取队列中的数据
 * @param p_packet_queue
 * @param p_out_pkt
 * @param block
 * @return
 */
int select_audio_packet_queue(PacketQueue *p_packet_queue, AVPacket *p_out_pkt, int block) {
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
            SDL_CondWait(p_packet_queue->cond, p_packet_queue->mutex);
        }
    }
    SDL_UnlockMutex(p_packet_queue->mutex);         //解🔐
    return ret;
}