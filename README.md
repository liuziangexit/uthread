# uthread
[![Build Status](https://github.com/liuziangexit/uthread/workflows/C%2fC%2b%2b+CI/badge.svg)](https://github.com/liuziangexit/uthread/actions)
<br>
Cross-platform implementation of user-space thread(As known as coroutine).
<br>
跨平台的用户态线程实现(也称作协程)。
<h2>Features / 功能</h2>
-Context switch / 上下文切换
<br>
-Automatic switch to other uthread while blocking on IO system call / 阻塞于IO系统调用时自动切换到其他uthread
<br>
-Cross platform(currently support Linux and Windows NT under x86 and x64) / 跨平台(目前支持x86和x64下的Linux和Windows NT)
<h2>Progress / 进展</h2>

| Platform | Feature | Status | Branch |
| -------- | ------- | ------ | ------ |
| Linux    |Context-Switch|OK |  master|
| Linux    |System call Hook|OK|master |
| Linux    |Hook Implementation|WORKING|dev-hookimpl|
| Linux    |  Demo   |PENDING |-|
|Windows NT|Context-Switch|PENDING|-|
|Windows NT|System call Hook|PENDING|-|
|Windows NT|Hook Implementation|PENDING|-|
|Windows NT|  Demo   |PENDING|-|
| macOS    |Context-Switch|PENDING|-|
| macOS    |System call Hook|PENDING|-|
| macOS    |Hook Implementation|PENDING|-|
| macOS    |  Demo   |PENDING |-|
| iOS      |Context-Switch|PENDING|-|
| iOS      |System call Hook|PENDING|-|
| iOS      |Hook Implementation|PENDING|-|
| iOS      |  Demo   |PENDING|-|
| FreeBSD  |Context-Switch|PENDING|-|
| FreeBSD  |System call Hook|PENDING|-|
| FreeBSD  |Hook Implementation|PENDING|-|
| FreeBSD  |  Demo   |PENDING|-|
| Solaris  |Context-Switch|PENDING|-|
| Solaris  |System call Hook|PENDING|-|
| Solaris  |Hook Implementation|PENDING|-|
| Solaris  |  Demo   |PENDING|-|
