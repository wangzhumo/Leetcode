//
// Created by 王诛魔 on 2019-07-26.
//

#include "yuv_play_sdl.h"

void yuv_player(char *file_name) {

    //input video file
    FILE *video_file = NULL;
    //Windows
    SDL_Window *sdl_window = NULL;
    //Render
    SDL_Renderer *sdl_renderer = NULL;
    //SDL_Event
    SDL_Event sdl_event;
    //SDL_texture
    SDL_Texture *sdl_texture = NULL;
    //SDL_Thread
    SDL_Thread *timer_thread = NULL;


    Uint32 pix_format = 0;
    Uint8 *video_pos = NULL;
    Uint8 *video_end = NULL;

    //常量
    const int video_width = 608;
    const int video_height = 398;

    //定义大小
    int windows_wight = 608;
    int windows_height = 398;


    //计算出一个yuv帧使用的长度
    //const unsigned int yuv_frame_length = (video_width * video_height) * 2;
    //yuv420  = y (video_width * video_height)  + (u/v (video_width * video_height)/2) * 2
    //此处为了按整数对齐.
    const unsigned int yuv_frame_length = (video_width * video_height) * 12 / 8;
    unsigned int temp_yuv_frame_length = yuv_frame_length;

    // 0000 1111 后4位如果 & 1111 不等于 0 则说明，yuv_frame_length 的后4位有不是1的。
    //  0000 0000 0000 0101 1000 1001 1110 0000      362976
    //  0000 0000 0000 0000 0000 0000 0000 1111      0xf
    //  0000 0000 0000 0000 0000 0000 0000 0000

    //  0000 0000 0000 0000 1111 1111 1111 0000      0xFFF0
    //  0000 0000 0000 0000 1000 1001 1110 0000
    //  0000 0000 0000 0000 0000 0000 0001 0000      0x10
    //  0000 0000 0000 0000 1000 1001 1111 0000

    if (yuv_frame_length & 0xF) {
        temp_yuv_frame_length = (yuv_frame_length & 0xFFF0) + 0x10;
    }











    //0.初始化SDL
    SDL_Init(SDL_INIT_VIDEO);

}