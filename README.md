# Cpp-AI-Web-Server

这是一个多功能AI平台，集成了 DeepSeek、SceniMeFy、Waifu Diffusion 等多款先进的 AI 模型。

demo：http://101.37.26.211:8000/

后端采用 Modern C++（GCC 15.1，C++23）高性能实现，主要特点如下：

1. **半同步半反应堆架构（Half‑Sync/Half‑Reactor）**
   - Reactor 层：基于 epoll 的非阻塞 I/O，引擎化地监听网络事件，负责接收客户端请求并发送响应。
   - Sync Queue + 线程池：将已接收的 I/O 事件推入同步队列，由工作线程池执行模型推理或其他计算任务，彻底解耦网络事件分发与业务逻辑执行。
2. **现代 C++ 线程池**
   - 泛型任务封装：使用可变参数模板与 `std::function<void()>`，支持任意可调用对象的提交与执行。
   - 高效参数传递：结合 `std::bind` 与完美转发 (`std::forward`)，在封装任务时保留左右值属性，最大限度减少拷贝。
   - 安全同步机制：RAII 风格的互斥锁（`std::unique_lock`）与条件变量（`std::condition_variable`）配合，既避免死锁，又能在无任务时让线程休眠，从而节省 CPU 资源。
3. **高效 HTTP 解析**
   - 有限状态机：基于状态转换的方式解析 HTTP 首部，性能优异且易于扩展。
   - Trie 路由：利用Trie结构加速路由查找，根据请求的 method 与 path，快速定位到对应的控制器。
4. **高度解耦的模块化设计**
   - 插件式控制器：只需在 control 目录下新增对应的 Controller，并在路由器（Router）中注册路径，即可无侵入地扩展新功能或接入新模型。
   - RAII 管理资源：各模块均以 RAII 风格实现，自动构造与析构，有效避免资源泄漏，提升系统稳定性。

