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
    if (!state->parse_demux_tid){
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

