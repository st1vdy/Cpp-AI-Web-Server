//
// Created by dongye on 25-5-3.
//
#pragma once
#include <sys/epoll.h>
#include <netinet/in.h>

#include <chrono>
#include <functional>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>

class Epoll {
public:
    explicit Epoll(size_t max_events)
        : epoll_fd_(epoll_create1(0)), events_(max_events) {
        if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1 failed");
    }
    ~Epoll() = default;

    void add(int fd, uint32_t events) const { ctl(fd, events, EPOLL_CTL_ADD); }
    void modify(int fd, uint32_t events) const { ctl(fd, events, EPOLL_CTL_MOD); }
    void remove(int fd) const { ctl(fd, 0, EPOLL_CTL_DEL); }

    int wait(int timeout_ms) { return epoll_wait(epoll_fd_, events_.data(), events_.size(), timeout_ms); }

    std::pair<int, uint32_t> event_at(int index) const {
        return { events_[index].data.fd, events_[index].events };
    }

private:
    void ctl(int fd, uint32_t events, int op) const {
        epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd_, op, fd, &ev) < 0) throw std::runtime_error("epoll_ctl failed");
    }

    int epoll_fd_;
    std::vector<epoll_event> events_;
};