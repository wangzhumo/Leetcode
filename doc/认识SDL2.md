## SDL

### SDL的下载与安装

#### 下载

http://www.libsdl.org/download-2.0.php



#### 安装

http://wiki.libsdl.org/Installation#Mac_OS_X

- 解压缩到目录

- 编译


``` 
./configure --prefix=/usr/local

......

sudo make -j 8 && make instal
```



### SDL在Cmake中引用

#### SDL2信息

- 找到sdl2的安装位置,移动到这个目录下

- 查看配置`vim ./lib/cmake/SDL2/sdl2-config.cmake`

  ```
  set(prefix "/usr/local")
  set(exec_prefix "${prefix}")
  set(libdir "${exec_prefix}/lib")
  set(SDL2_PREFIX "/usr/local")
  set(SDL2_EXEC_PREFIX "/usr/local")
  set(SDL2_LIBDIR "${exec_prefix}/lib")
  set(SDL2_INCLUDE_DIRS "${prefix}/include/SDL2")
  set(SDL2_LIBRARIES "-L${SDL2_LIBDIR}  -lSDL2")
  string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
  
  ```

  可以看到各个位置

  

#### 完成Cmake

##### 创建FindSDL2.cmake

```
cd {project}
mkdir ./cmake
cd ./cmake
mkdir ./modules
vim ./FindSDL2.cmake
```

内容过长,不重复了.`cmake/modules/FindSDL2.cmake`



##### 编辑CMakeLists.text

```
# Add the 'Modules' folder to the search path for FindXXX.cmake files
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(SDL2 REQUIRED)

include_directories(
        ${SDL2_INCLUDE_DIR}
)

target_link_libraries(Media
        ${SDL2_LIBRARY}
)
```



## SDL的基本使用

#### SDL_Init(SDL_INIT_VIDEO);

初始化SDL

#### SDL_CreateWindow 创建Windows

 * @param Title, The title of the window, in UTF-8 encoding.
 * @param x,     The x position of the window
 * @param y,     The y position of the window
 * @param w,     The width of the window, in screen coordinates.
 * @param h,     The height of the window, in screen coordinates.
 * @param flags, The flags for the window



#### SDL_CreateRenderer  创建Render

* @param window,  The window where rendering is displayed.

* @param index    The index of the rendering driver to initialize, or -1 to 

  initialize the first one supporting the requested flags.

* @param flags    ::SDL_RendererFlags.



#### SDL_RenderPresent

提送renderer到图形引擎,显示到显示器



#### 销毁

- SDL_DestroyWindow
- SDL_DestroyRenderer