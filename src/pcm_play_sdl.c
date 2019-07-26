//
// Created by 王诛魔 on 2019/7/27.
//

#include "pcm_play_sdl.h"

#define BLOCK_SIZE 4096000

void read_audio_data(){

}


int play_pcm_audio(char *audio_path) {

    int result = -1;
    FILE *audio_file = NULL;
    Uint8 *audio_buffer = NULL;


    //0.init SDL
    if (SDL_Init(SDL_INIT_AUDIO)) {
        printf("Failed to Init. \n");
        return;
    }


    //1.open audio file
    audio_file = fopen(audio_path, "r");
    if (!audio_file) {
        printf("Failed to open audio file &s. \n", audio_path);
        goto __EXIT;
    }

    //2.create audio buffer
    audio_buffer = (Uint8 *) malloc(BLOCK_SIZE);
    if (!audio_buffer) {
        printf("Failed to alloc buffer memory. \n");
        goto __EXIT;
    }

    //3.create audio device
    SDL_AudioSpec sdl_audioSpec;
    sdl_audioSpec.freq = 44100;
    sdl_audioSpec.channels = 1;
    //** change this
    sdl_audioSpec.format = AUDIO_S16SYS;
    //回调函数,如果声卡需要数据,则在callback设置
    sdl_audioSpec.callback = read_audio_data;
    sdl_audioSpec.userdata = NULL;

    //** open audio device
    if (SDL_OpenAudio(&sdl_audioSpec, NULL)){
        //0  成功
        printf("Failed to open audio device \n");
        goto __EXIT;
    }

    //4.play/pause audio
    // 0 - 暂停
    // 1 - 播放
    SDL_PauseAudio(0);




    __EXIT:
    if (audio_file) {
        fclose(audio_file);
    }
    if (audio_buffer) {
        free(audio_buffer);
    }
    SDL_CloseAudio();
    SDL_Quit();
}