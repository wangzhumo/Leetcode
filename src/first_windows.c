//
// Created by 王诛魔 on 2019/7/26.
//
#include "first_windows.h"


void init_sdl_window() {

    //Windows
    SDL_Window *sdl_window = NULL;
    //Render
    SDL_Renderer *sdl_renderer = NULL;



    SDL_Init(SDL_INIT_VIDEO);


    //1.创建窗口
    printf("Create SDL2 Windows. \n");
    /*
     * @param Title, The title of the window, in UTF-8 encoding.
     * @param x,     The x position of the window
     * @param y,     The y position of the window
     * @param w,     The width of the window, in screen coordinates.
     * @param h,     The height of the window, in screen coordinates.
     * @param flags, The flags for the window
     *
     */
    sdl_window = SDL_CreateWindow(
            "SDL2 Windows.",
            300,
            400,
            640,
            480,
            SDL_WINDOW_SHOWN
    );

    if (!sdl_window) {
        printf("Fail to create SDL2 windows.");
        goto __EXIT;
    }

    //2.创建Render
    /*
    * @param window,  The window where rendering is displayed.
    * @param index    The index of the rendering driver to initialize, or -1 to
    *                  initialize the first one supporting the requested flags.
    * @param flags    ::SDL_RendererFlags.
    */
    printf("Create SDL2 Renderer. \n");
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);

    if (!sdl_renderer) {
        printf("Fail to create SDL2 sdl_renderer.");
        goto __DESTROY_WINDOWS;
    }

    //设置颜色/置为空
    SDL_SetRenderDrawColor(sdl_renderer, 255, 100, 100, 00);
    SDL_RenderClear(sdl_renderer);


    //3.推送到图形引擎,显示到屏幕
    SDL_RenderPresent(sdl_renderer);


    SDL_Delay(3000);


    __DESTROY_WINDOWS:
    SDL_DestroyWindow(sdl_window);
    printf("Destroy SDL2 windows.");

    __EXIT:
    SDL_DestroyRenderer(sdl_renderer);
    SDL_Quit();
}