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
