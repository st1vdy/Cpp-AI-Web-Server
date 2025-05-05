//
// Created by dongye on 25-5-3.
//

#include "server.h"

Server::Server(int port) :
        timer_fd_(timerfd_create(CLOCK_MONOTONIC, 0)),
        epoll_(1024),
        http_pool_(4),
        llm_pool_(4),
        running_(true) {
    user_db_["22351056@zju.edu.cn"] = std::hash<std::string>()("114514");

    setup_signal_handlers();

    register_router();

    setup_timer();

    acceptor_ = std::make_unique<Acceptor>(
        port,
        epoll_,
        [this](FdWrapper&& client_fd, sockaddr_in peer) {
            this->handle_accept(std::move(client_fd), peer);
        }
    );

    print_info(std::format("Server listening on port {}.", port));
}

Server::~Server() {  }

void Server::setup_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = &Server::handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // 避免被信号打断的系统调用直接返回错误

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Error setting SIGINT handler: " << strerror(errno) << "\n";
    }
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "Error setting SIGTERM handler: " << strerror(errno) << "\n";
    }
}

sig_atomic_t Server::signalReceived_ = 0;

void Server::handle_signal(int signo) {
    signalReceived_ = signo;  // 只做最小工作：记录下收到的信号编号
}

void Server::handle_accept(FdWrapper&& client_fd, sockaddr_in peer) {
    print_info(std::format("New connection from {}:{}", inet_ntoa(peer.sin_addr), peer.sin_port));
    // 插入会话管理，并添加 EPOLLIN | EPOLLET
    int fd = client_fd.get();
    sessions_.emplace(fd, HttpSession(std::move(client_fd)));
    epoll_.add(fd, EPOLLIN | EPOLLET);
}

void Server::setup_timer() const {
    struct itimerspec its{0};
    its.it_interval.tv_sec = 10;
    its.it_value.tv_sec = 15;
    if (timer_fd_.get() < 0) throw std::runtime_error("timerfd create failed!");
    timerfd_settime(timer_fd_.get(), 0, &its, nullptr);
    epoll_.add(timer_fd_.get(), EPOLLIN);
}

void Server::register_router() {
    router_.register_route("POST", "/login", LoginController::login_handle);
    router_.register_route("POST", "/register", LoginController::register_handle);
    router_.register_route("POST", "/chat", LLMController::chat_handle);
    router_.register_route("POST", "/style-transfer", ScenimefyController::scenimefy_handle);
    router_.register_route("POST", "/waifu/gen", WaifuController::generate_handler);
    router_.register_route("GET", "/style-transfer/sample", ScenimefyController::sample_handle);
    router_.register_route("GET", "/waifu/rd", WaifuController::random_prompts_handler);
    router_.register_route("GET", "/check_auth", LoginController::check_auth_token);
}

void Server::run() {
    while (running_) {
        int n = epoll_.wait(1000);
        for (int i = 0; i < n; i++) {
            if (auto [fd, events] = epoll_.event_at(i); fd == acceptor_->fd()) {
                acceptor_->accept_loop();
            } else if (fd == timer_fd_.get()) {
                // uint64_t expir;
                // read(timer_fd_.get(), &expir, sizeof(expir));
                // cleanup_loop(); TODO
            } else {
                epoll_.remove(fd);
                http_pool_.enqueue(&Server::handle_client, this, fd);
            }
        }
        if (signalReceived_ != 0) {
            print_info(std::format("Received signal {}, shutting down...", signalReceived_));
            running_ = false;
        }
    }
    print_info("Server shutting down.");
}

void Server::handle_client(int client_fd) {
    print_info("Start handling client...");
    auto&& it = sessions_.find(client_fd);
    if (it == sessions_.end()) return;  // 已被清理

    HttpSession& session = it->second;
    make_blocking(it->first);

    // 1. 读 header/body
    HttpRequest req;
    if (!session.read_header(req)) {
        print_info("No header.");
        sessions_.erase(it);
        return;
    }
    print_info(std::format("Http header: {}", session.get_header()));
    print_info("Start read body...");
    session.read_body(req);
    // print_info(std::format("Http body: {}", session.get_body()));

    // 2. 同步生成响应
    HttpResponse resp = router_.route(req);

    // 3. 发送响应 & 清理
    session.send_response(resp.to_response_string());
    sessions_.erase(client_fd);
}

