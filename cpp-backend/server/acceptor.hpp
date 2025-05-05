//
// Created by dongye on 25-5-3.
//

#pragma once
#include <fcntl.h>

#include "epoll.hpp"
#include "../utils/utils.hpp"

class Acceptor {
public:
    Acceptor(uint16_t port,
             Epoll& epoll,
             std::function<void(FdWrapper&&, sockaddr_in)> on_new) :
            listener_(socket(AF_INET, SOCK_STREAM, 0)),
            epoll_(epoll),
            new_accept_(std::move(on_new)) {
        if (!listener_) throw std::runtime_error("socket() failed");

        int opt = 1;
        setsockopt(listener_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        make_nonblocking(listener_.get());

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(listener_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");
        if (listen(listener_.get(), SOMAXCONN) < 0)
            throw std::runtime_error("listen() failed");

        epoll_.add(listener_.get(), EPOLLIN);
    }

    ~Acceptor() = default;

    int fd() const { return listener_.get(); }

    void accept_loop() {
        while (true) {
            sockaddr_in peer{};
            socklen_t len = sizeof(peer);
            int client_fd = accept(listener_.get(), reinterpret_cast<sockaddr*>(&peer), &len);
            if (client_fd < 0) break;

            FdWrapper client(client_fd);
            make_nonblocking(client.get());
            new_accept_(std::move(client), peer);
        }
    }

private:
    FdWrapper listener_;
    Epoll& epoll_;
    std::function<void(FdWrapper&&, sockaddr_in)> new_accept_;
};