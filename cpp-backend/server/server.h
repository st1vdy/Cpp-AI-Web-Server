//
// Created by dongye on 25-5-3.
//

#ifndef SERVER_H
#define SERVER_H

#pragma once
#include <signal.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <string.h>

#include <atomic>
#include <unordered_map>
#include <map>

#include "../thread/thread_pool.hpp"
#include "../log/logger.hpp"
#include "../http/http_session.h"
#include "../http/http_message.hpp"
#include "../utils/utils.hpp"
#include "../control/login_controller.hpp"
#include "../control/llm_controller.hpp"
#include "../control/scenimefy_controller.hpp"
#include "epoll.hpp"
#include "acceptor.hpp"
#include "router.hpp"

class Server {
public:
    explicit Server(int port);
    ~Server();
    void run();
private:
    std::atomic<bool> running_;
    FdWrapper timer_fd_;
    std::unordered_map<std::string, size_t> user_db_;
    std::map<int, HttpSession> sessions_;
    Epoll epoll_;
    Router router_;
    ThreadPool http_pool_;
    ThreadPool llm_pool_;
    std::unique_ptr<Acceptor> acceptor_;

    void handle_accept(FdWrapper&& client_fd, sockaddr_in peer);
    //void cleanup_loop();
    void handle_client(int client_fd);
    void setup_timer() const;

    void register_router();

    static void setup_signal_handlers();
    static void handle_signal(int signo);
    static sig_atomic_t signalReceived_;
};



#endif //SERVER_H
