//
// Created by 王诛魔 on 2019/7/26.
//

#include <stdio.h>

int main(int argc, char *args[]) {
    //常量
    const int video_width = 608;
    const int video_height = 398;

    int value = video_height * video_width * 12 / 8;


    printf("%d \n",value);

    printf("%d \n", value & 0xF);

    return 0;
}