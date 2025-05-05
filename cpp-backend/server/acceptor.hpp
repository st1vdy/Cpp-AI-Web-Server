//
// Created by dongye on 25-5-3.
//

#pragma once
#include <fcntl.h>

#include "epoll.hpp"
#include "../utils/utils.hpp"

class Acceptor {
public:
    /// @param port    要监听的 TCP 端口
    /// @param epoll   用于注册/注销监听 fd
    /// @param on_new  当 accept 到一个新的连接时调用：接受一个 FdWrapper 和 peer 地址
    Acceptor(uint16_t port,
             Epoll& epoll,
             std::function<void(FdWrapper&&, sockaddr_in)> on_new) :
            listener_(socket(AF_INET, SOCK_STREAM, 0)),
            epoll_(epoll),
            new_accept_(std::move(on_new)) {
        if (!listener_) throw std::runtime_error("socket() failed");

        // 设置 SO_REUSEADDR
        int opt = 1;
        setsockopt(listener_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        // 设置非阻塞
        make_nonblocking(listener_.get());

        // bind + listen
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(listener_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");
        if (listen(listener_.get(), SOMAXCONN) < 0)
            throw std::runtime_error("listen() failed");

        // 注册到 epoll
        epoll_.add(listener_.get(), EPOLLIN);
    }

    ~Acceptor() = default;  // listener_ 的析构会自动 close(fd)

    /// 返回原始 listener fd，用于 epoll 识别
    int fd() const { return listener_.get(); }

    /// 当 epoll 检测到 listener_ 可读时，循环 accept 所有挂起连接
    void accept_loop() {
        while (true) {
            sockaddr_in peer{};
            socklen_t len = sizeof(peer);
            int client_fd = accept(listener_.get(), reinterpret_cast<sockaddr*>(&peer), &len);
            if (client_fd < 0) break;  // 全部 accept 完毕

            // 将 client_fd 包装到 FdWrapper
            FdWrapper client(client_fd);
            // 设置非阻塞
            make_nonblocking(client.get());

            // 回调，将 client 的所有权移交给上层
            new_accept_(std::move(client), peer);
        }
    }

private:
    FdWrapper listener_;
    Epoll& epoll_;
    std::function<void(FdWrapper&&, sockaddr_in)> new_accept_;
};