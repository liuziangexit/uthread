# uthread
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