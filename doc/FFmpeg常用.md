## FFmpeg常用命令

#### 基本格式

```
ffmpeg [global_options] {[input_file_options] -i input_url} ...
						 {[output_file_options] output_url} ...

```

#### 抽取YUV

```
ffmpeg -i ~/media/input.mp4 -an -c:v rawvideo -pixel_format yuv420p ~/media/out.yuv


```