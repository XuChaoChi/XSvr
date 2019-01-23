# XSvr

## 简介

一个依赖很多三方库的 **linux c++ 服务端** 程序
并且对三方库进行了封装
同时自己也封装了些工具类
只要丰富logic即可

## 三方库

### fmt

可以直接使用

### libevent 2.1.8

编译库文件

    $ wget https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz
    $ tar -zxvf libevent-2.1.8-stable.tar.gz
    $ cd libevent-2.1.8-stable/
    $ ./configure
    $ make
    $ make verify   # (optional)
    $ sudo make install

### protobuf 3.6.1

    $ wget https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-all-3.6.1.tar.gz
    $ tar -zxvf protobuf-all-3.6.1.tar.gz
    $ cd protobuf-3.6.1/
    $ ./configure
    $ make
    $ make check
    $ sudo make install
    $ sudo ldconfig # refresh shared library cache.

### spdlog 1.2.6

    头文件组成的库可以直接用

### mysqlclient

    $ wget https://cdn.mysql.com//Downloads/MySQL-8.0/mysql-8.0.13.tar.gz
    $ tar -zxvf mysql-8.0.13.tar.gz
    $ cd mysql-8.0.13/
    $ build build
    $ cd build/
    $ cmake ..
    $ make
    $ sudo make install

## 使用

## TODO
- timer
- hiredis helper
- json config reader
- file helper
- rpc
- http
- epoll
- googletest
