//
// Created by 王诛魔 on 2019-07-26.
//

#include "texture_sdl.h"


void start_sdl_texture() {
    //flag quit
    int quit = 1;
    SDL_Rect rect;
    rect.w = 35;
    rect.h = 30;

    //Windows
    SDL_Window *sdl_window = NULL;
    //Render
    SDL_Renderer *sdl_renderer = NULL;
    //SDL_Event
    SDL_Event sdl_event;
    //SDL_texture
    SDL_Texture *sdl_texture = NULL;

    //0.初始化SDL
    SDL_Init(SDL_INIT_VIDEO);

    //1.创建窗口
    printf("Create SDL2 Windows. \n");
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
    printf("Create SDL2 Renderer. \n");
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);

    if (!sdl_renderer) {
        printf("Fail to create SDL2 sdl_renderer.");
        goto __DESTROY_WINDOWS;
    }

    //3.创建texture
    /*
     * @param renderer The renderer.
     * @param format   The format of the texture.
     * @param access   One of the enumerated values in ::SDL_TextureAccess.
     *                          SDL_TEXTUREACCESS_STATIC	：变化极少
     *                          SDL_TEXTUREACCESS_STREAMING	：变化频繁
     *                          SDL_TEXTUREACCESS_TARGET	：普通
     * @param w        The width of the texture in pixels.
     * @param h        The height of the texture in pixels.
     */
    sdl_texture = SDL_CreateTexture(
            sdl_renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_TARGET,
            600,
            500
    );

    if (!sdl_texture) {
        printf("Fail to create SDL2 sdl_texture.");
        goto __DESTROY_RENDER;
    }


    do {
        //写入event到sdl_event
        SDL_WaitEvent(&sdl_event);
        //SDL_WaitEvent(&sdl_event);
        //这个Wait会限制调用的频率。

        switch (sdl_event.type) {
            case SDL_QUIT:
                quit = 0;
                break;
            default:
                SDL_Log("SDL_Event type : %d", sdl_event.type);
        }

        //4.创建方框，while保证一直不停的移动.

        rect.x = rand() % 600;
        rect.y = rand() % 500;

        //设置render到texture上。
        SDL_SetRenderTarget(sdl_renderer, sdl_texture);
        //设置颜色
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, 0);
        //复写
        SDL_RenderClear(sdl_renderer);

        //在Render中绘制一个rect
        SDL_RenderDrawRect(sdl_renderer, &rect);
        //设置纹理颜色
        SDL_SetRenderDrawColor(sdl_renderer, 0, 255, 0, 0);
        //使用纹理填充FillRect，rect
        SDL_RenderFillRect(sdl_renderer, &rect);

        //切换回原来的画布
        SDL_SetRenderTarget(sdl_renderer, NULL);
        //纹理输出到图形设备 ->用于计算
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);

        //上屏 -> 显示到屏幕
        SDL_RenderPresent(sdl_renderer);

    } while (quit);


    SDL_DestroyTexture(sdl_texture);

    __DESTROY_RENDER:
    SDL_DestroyRenderer(sdl_renderer);


    __DESTROY_WINDOWS:
    SDL_DestroyWindow(sdl_window);
    printf("Destroy SDL2 windows.");

    __EXIT:
    SDL_Quit();
}