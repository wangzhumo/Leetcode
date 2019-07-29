//
// Created by 王诛魔 on 2019/7/27.
//

#include "pcm_play_sdl.h"

#define BLOCK_SIZE 4096000

static Uint8 *audio_buffer = NULL;
static size_t audio_buff_len = 0;
static Uint8 *audio_pos = NULL;

/**
 * 播放设备的回调,调取数据
 * @param udata  [sdl_audioSpec.userdata]
 * @param stream 音频设备的buffer
 * @param length
 */
void read_audio_data(void *udata, Uint8 *stream, int length) {
    if (audio_buff_len == 0) {
        //读取完毕,直接返回
        return;
    }

    //清空
    SDL_memset(stream, 0, length);

    //长度谁小用谁.
    length = length < audio_buffer ? length : audio_buff_len;

    //copy 拷贝数据到stream中,从audio_pos地址中,拷贝长度 length
    //SDL_MIX_MAXVOLUME 音量.
    SDL_MixAudio(stream, audio_pos, length, SDL_MIX_MAXVOLUME);

    //拷贝完毕,则audio_pos应该移动 length 的位置.
    audio_pos += length;

    //可用的数据长度减少了.
    audio_buff_len -= length;
}


/**
 * 播放音频.
 * @param audio_path
 * @return int
 */
int play_pcm_audio(char *audio_path) {

    int result = -1;
    FILE *audio_file = NULL;


    //0.init SDL
    if (SDL_Init(SDL_INIT_AUDIO)) {
        printf("Failed to Init. \n");
        return result;
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
    sdl_audioSpec.channels = 2;
    //** change this
    sdl_audioSpec.format = AUDIO_S16SYS;
    //回调函数,如果声卡需要数据,则在callback设置
    sdl_audioSpec.callback = read_audio_data;
    sdl_audioSpec.userdata = NULL;

    //** open audio device
    if (SDL_OpenAudio(&sdl_audioSpec, NULL)) {
        //0  成功
        printf("Failed to open audio device \n");
        goto __EXIT;
    }

    //4.play/pause audio
    // 0 - 暂停
    // 1 - 播放
    SDL_PauseAudio(0);

    do {
        //** read audio file  -> audio_buffer.
        audio_buff_len = fread(audio_buffer, 1, BLOCK_SIZE, audio_file);
        audio_pos = audio_buffer;
        //audio_pos 指向了audio_buffer的头
        //audio_buffer + audio_buff_len  = 整个buffer的尾部.
        while (audio_pos < (audio_buffer + audio_buff_len)) {
            SDL_Delay(1);
        }
    } while (audio_buff_len != 0);

    result = 0;

    __EXIT:
    if (audio_file) {
        fclose(audio_file);
    }
    if (audio_buffer) {
        free(audio_buffer);
    }
    SDL_CloseAudio();
    SDL_Quit();
    return result;
}