# 基于C++11实现muduo网络库

### 1.组件

- 小组件

  - noncopyable

  - Logger

  - Timestamp

- Reactor

  - Channel

  - Poller

  - EpollPoller

- Threaad

  - Thread

  - EventLoopThread

  - EventLoopThreadPoll

- Acceptor

  - Socket

  - Acceptor

- TcpConnection
  - Buffer

- TcpServer

### 2.安装方法

```shell
./autobuild.sh
```

### 3.Echo服务器

```shell
cd example
make clean
make
./testserver

telnet 127.0.0.1 8000
```

