set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()

Plugin 'VundleVim/Vundle.vim'
Plugin 'vim-airline/vim-airline'
Plugin 'vim-airline/vim-airline-themes'
Plugin 'tpope/vim-fugitive'
Plugin 'git://git.wincent.com/command-t.git'
Plugin 'rstacruz/sparkup', {'rtp': 'vim/'}
Plugin 'octol/vim-cpp-enhanced-highlight'   "C++11/14
Plugin 'nathanaelkane/vim-indent-guides'
Plugin 'Valloric/YouCompleteMe'
Plugin 'ybian/smartim'
Plugin 'tomasr/molokai'
Plugin 'scrooloose/nerdtree'

call vundle#end()            " required


""""""""实用设置""""""
filetype plugin indent on    " required

" 语法高亮
syntax off
syntax enable 

" 突出显示当前行/列
set cursorline              
set cursorcolumn
" 显示光标当前位置
set ruler
" 显示行号
set number
" 显示颜色
set t_Co=256
set laststatus=2
" 高亮搜索
set hlsearch

" 去掉输入错误的提示声音
set noeb
" 自动缩进
set autoindent
" Tab键的宽度
set tabstop=4
" 统一缩进为4
set softtabstop=4
"编码设置
set enc=utf-8
set fencs=utf-8,ucs-bom,shift-jis,gb18030,gbk,gb2312,cp936
"语言设置
set langmenu=zh_CN.UTF-8
set helplang=cn
set laststatus=1

" vim theme"
colorscheme molokai

" NERDTree"
let NERDChristmasTree=0
let NERDTreeWinSize=35
let NERDTreeChDirMode=2
let NERDTreeIgnore=['\~$', '\.pyc$', '\.swp$']
let NERDTreeShowBookmarks=1
let NERDTreeWinPos="left"
let g:nerdtree_tabs_open_on_gui_startup=1
let g:nerdtree_tabs_open_on_console_startup=1

autocmd vimenter * if !argc() | NERDTree | endif
autocmd bufenter * if (winnr("$") == 1 && exists("b:NERDTreeType") && b:NERDTreeType == "primary") | q | endif


" YCM 补全菜单配色
" 菜单
"highlight Pmenu ctermfg=2 ctermbg=3 guifg=#005f87 guibg=#EEE8D5
" 选中项
"highlight PmenuSel ctermfg=2 ctermbg=3 guifg=#AFD700 guibg=#106900
" 补全功能在注释中同样有效
let g:ycm_complete_in_comments=1
" 允许 vim 加载 .ycm_extra_conf.py 文件，不再提示
let g:ycm_confirm_extra_conf=0
" 开启 YCM 标签补全引擎
let g:ycm_collect_identifiers_from_tags_files=1
" 补全内容不以分割子窗口形式出现，只显示补全列表
"set completeopt-=preview
" 从第一个键入字符就开始罗列匹配项
let g:ycm_min_num_of_chars_for_completion=1
" 禁止缓存匹配项，每次都重新生成匹配项
let g:ycm_cache_omnifunc=0
" 语法关键字补全			
let g:ycm_seed_identifiers_with_syntax=1
" load conf
let g:ycm_global_ycm_extra_conf='~/.vim/.ycm_extra_conf.py'

" 退格可用
set backspace=2

" 随 vim 自启动
let g:indent_guides_enable_on_vim_startup=1
" 从第二层开始可视化显示缩进
let g:indent_guides_start_level=2
" 色块宽度
let g:indent_guides_guide_size=1


" 快捷键
map <F3> :NERDTreeMirror<CR>
map <F3> :NERDTreeToggle<CR>
