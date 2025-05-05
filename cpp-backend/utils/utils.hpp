//
// Created by dongye on 25-5-3.
//

#pragma once
#include <random>

#include  "fcntl.h"

inline std::mt19937 gen(114514);
inline int rand_int(int l, int r) {
    return std::uniform_int_distribution<int>(l, r)(gen);
}

class FdWrapper {
public:
    explicit FdWrapper(int fd = -1) : fd_(fd) {}
    ~FdWrapper() { if (fd_ >= 0) close(fd_); }
    FdWrapper(const FdWrapper&) = delete;
    FdWrapper& operator=(const FdWrapper&) = delete;
    FdWrapper(FdWrapper&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    FdWrapper& operator=(FdWrapper&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
    int get() const { return fd_; }
    explicit operator bool() const { return fd_ >= 0; }
private:
    int fd_;
};

inline void make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

inline void make_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

inline std::string trim(const std::string& s) {
    auto l = s.find_first_not_of(" \t\r\n");
    auto r = s.find_last_not_of (" \t\r\n");
    if (l == std::string::npos) return "";
    return s.substr(l, r - l + 1);
}
