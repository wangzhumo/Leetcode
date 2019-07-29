//
// Created by 王诛魔 on 2019-07-26.
//
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define QUIT_EVENT  (SDL_USEREVENT + 2)


#include "yuv_play_sdl.h"

int thread_exit_flag = 0;


/**
 * 定时刷新video数据 - 函数
 */
int refresh_video_timer(void *udata) {
    thread_exit_flag = 0;

    //while循环，刷新数据
    while (!thread_exit_flag) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }

    //退出循环后重置
    thread_exit_flag = 0;

    //退出循环，停止刷新video数据
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);
    return 0;
}


int yuv_player(char *file_path) {

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
    //目标大小
    SDL_Rect sdl_rect;

    //texture format 格式
    Uint32 pix_format = 0;

    //video position 容器
    Uint8 *video_pos = NULL;
    Uint8 *video_end = NULL;
    Uint8 *video_buffer = NULL;

    //成功读取的buffer长度
    unsigned int video_buff_len = 0;

    //常量
    const int video_width = 1920;
    const int video_height = 1080;

    //定义大小
    int windows_wight = 1920;
    int windows_height = 1080;


    //计算出一个yuv帧使用的长度
    //const unsigned int yuv_frame_length = (video_width * video_height) * 2;
    //yuv420  = y (video_width * video_height)  + (u/v (video_width * video_height)/2) * 2
    //此处为了按整数对齐.
    const unsigned int yuv_frame_length = (video_width * video_height) * 12 / 8;
    unsigned int temp_yuv_frame_length = yuv_frame_length;


    //0.初始化SDL
    printf("Initialize SDL2. \n");
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Could not initialize SDL2 - %s \n", SDL_GetError());
        return -1;
    }

    //1.创建一个Windows.
    printf("Create SDL2 Windows. \n");
    sdl_window = SDL_CreateWindow(
            "YUV Media Player",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            windows_wight,
            windows_height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!sdl_window) {
        fprintf(stderr, "Fail to create SDL2 windows.");
        goto __FAIL;
    }

    //2.创建一个Render
    printf("Create SDL2 Renderer. \n");
    sdl_renderer = SDL_CreateRenderer(
            sdl_window,
            -1,
            0
    );

    if (!sdl_renderer) {
        fprintf(stderr, "Fail to create SDL2 sdl_renderer.");
        goto __FAIL;
    }

    //3.创建一个texture
    printf("Create SDL2 Texture. \n");
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pix_format = SDL_PIXELFORMAT_IYUV;


    sdl_texture = SDL_CreateTexture(
            sdl_renderer,
            pix_format,
            SDL_TEXTUREACCESS_STREAMING,
            video_width,
            video_height
    );


    //4.读取yuv文件.
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

    //** alloc space  temp_yuv_frame_length > yuv_frame_length
    video_buffer = (Uint8 *) malloc(temp_yuv_frame_length);
    if (!video_buffer) {
        fprintf(stderr, "Failed to alloc yuv frame buffer. \n");
        goto __FAIL;
    }

    //** open yuv file
    video_file = fopen(file_path, "r");
    if (!video_file) {
        fprintf(stderr, "Failed to open yuv file %s \n", file_path);
        goto __FAIL;
    }

    /*
     * @param buffer	-	pointer to the array where the read objects are stored
     * @param size	-	size of each object in bytes,一个单元的大小
     * @param count	-	the number of the objects to be read，需要读取的单元数量
     * @param stream	-	the stream to read
     *
     * @return Number of objects read successfully
     */
    //** read yuv file
    if ((video_buff_len = fread(video_buffer, 1, yuv_frame_length, video_file)) <= 0) {
        fprintf(stderr, "Failed to read data from yuv file!\n");
        goto __FAIL;
    }

    //** video position info   [pos,end]
    video_pos = video_buffer;
    video_end = video_buffer + video_buff_len;

    //5.创建一个Timer_Thread,用于发送视频数据信号
    printf("Create Timer Thread. \n");
    timer_thread = SDL_CreateThread(
            refresh_video_timer,
            NULL,
            NULL
    );


    do {
        SDL_WaitEvent(&sdl_event);
        switch (sdl_event.type) {
            case REFRESH_EVENT:
                //更新
                SDL_UpdateTexture(
                        sdl_texture,
                        NULL,
                        video_pos,
                        video_width
                );

                //** Copy to device
                sdl_rect.x = 0;
                sdl_rect.y = 0;
                sdl_rect.w = windows_wight;  //SDL_GetWindowSize
                sdl_rect.h = windows_height;
                SDL_RenderClear(sdl_renderer);
                SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
                //** show screen
                SDL_RenderPresent(sdl_renderer);

                //** read next yuv frame
                if ((video_buff_len = fread(video_buffer, 1, yuv_frame_length, video_file)) <= 0) {
                    thread_exit_flag = 1;
                    continue;
                }

                break;
            case SDL_WINDOWEVENT:
                //Resize
                printf("Event WindowSize. \n");
                SDL_GetWindowSize(sdl_window, &windows_wight, &windows_height);
                break;
            case SDL_QUIT:
                printf("Quit. \n");
                thread_exit_flag = 1;
                break;
            case QUIT_EVENT:
                printf("Quit. \n");
                thread_exit_flag = 1;
                goto __FAIL;
            default:
                break;
        }
    } while (1);

    __FAIL:
    //关闭
    if (video_file) {
        fclose(video_file);
    }


    SDL_Quit();
    return 0;
}
