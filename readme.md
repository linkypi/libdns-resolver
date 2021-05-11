## 基于 libevent 实现的 dns 异步解析

该项目是为了尝试解决rocketmq-4.8.0存在的一个bug而写，对应的  [issue](https://github.com/apache/rocketmq/issues/2697) 。由于java没有相应的网络库实现异步解析DNS的功能，故借助libevent来实现，该项目写好后需要由Java通过jna的方式调用，对应的Java项目是[jna-dns-resolve](https://github.com/linkypi/jna-dns-resolve) . 在使用 libevent 前需要在指定的平台编译出相关的类库后方可使用。

### 1. nmake 编译（不推荐）

从官网下载 [libevent-2.1.12--stable](https://libevent.org/) ，解压文件后在win10 操作系统打开 Developer Command Prompt.. 命令行窗口。定位到源码位置后执行命令：

``` shell
nmake /f Makefile.nmake
```

如出现以下错误则需要修改文件 minheap-internal.h ， 在其头部添加 #include "stdint.h" 即可。添加完成后继续执行编译命令。编译完成后会生成 obj 及 lib 的后缀文件：

```
error C2065: "UINT32_MAX": 未声明的标识符
```



### 2 使用 cmake 编译

使用该方式编译会生成对应的VS项目，非常方便。同时可以很快基于生成的项目来新建项目直接开发，省去各种依赖设置。在解压出来的libevent文件夹中创建build文件夹，进入文件夹后执行命令

``` shell
cmake .. -DEVENT__DISABLE_OPENSSL=ON -DEVENT__LIBRARY_TYPE=STATIC -DEVENT__DISABLE_DEBUG_MODE=ON -DCMAKE_BUILD_TYPE=Debug
```

执行完成后会在 build 文件夹下生成 VS 相关项目，然后再执行以下命令生成依赖包：

``` shell
cmake --build . --config Debug   # 根据需要生成指定版本动态库
cmake --build . --config Release
```



### 3. 基于生成项目开发 dns-resolver

打开生成的vs项目，找到 dns-example 右键生成。生成成功后即可在解决方案中新建项目 lib-dns 用于 Java端调用。右键新建项目，创建window 向导，下一步选择空项目， Dll 。完成后点击项目新建项 EvDnsResolver.h, EvDnsResolver.cpp. 添加完成后点击引用，添加event_core_static 及 event_extra_static引用。然后点击右键属性，在配置属性 -> C/C++ -> 常规里面设置附加包含项目：

```sh
G:\libevent-2.1.12-stable\build\include
G:\libevent-2.1.12-stable\include
G:\libevent-2.1.12-stable\compat
G:\libevent-2.1.12-stable\.\WIN32-Code
```

然后在 C/C++下方的链接器 -> 输入中 设置附加依赖项( 不一定全要，自行斟酌)：

``` sh
ws2_32.lib
shell32.lib
advapi32.lib
iphlpapi.lib
kernel32.lib
user32.lib
gdi32.lib
winspool.lib
ole32.lib
oleaut32.lib
uuid.lib
comdlg32.lib
```

万事俱备只欠东风，打开libevent官方文档，在 [DNS相关文档](http://www.wangafu.net/~nickm/libevent-book/Ref9_dns.html) 找到 Nonblocking lookups with evdns_getaddrinfo 。其实文档写得很清楚，也很容易阅读。此处使用的是 evdns_getaddrinfo 进行异步解析，而为了更细粒度控制(more fine-grained control than)，这里使用的是  evdns_base_resolve_ipv4 （Low-level DNS interfaces）

