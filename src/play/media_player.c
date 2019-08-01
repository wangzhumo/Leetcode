//
// Created by 王诛魔 on 2019/8/1.
//

#include "media_player.h"


int play(char *path) {

    SDL_Event event;
    VideoState *state = NULL;
    SDL_Window *p_sdl_window;
    SDL_Renderer *p_sdl_renderer;
    SDL_Texture *p_sdl_texture;

    //init VideoState
    state = av_malloc(sizeof(VideoState));
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

    if(!p_sdl_window) {
        fprintf(stderr, "SDL: could not set video mode:%s - exiting.\n", SDL_GetError());
        exit(1);
    }

    //create renderer
    p_sdl_renderer = SDL_CreateRenderer(p_sdl_window, -1, 0);
    if(!p_sdl_renderer) {
        fprintf(stderr, "SDL: could not create renderer.\n");
        exit(1);
    }


}