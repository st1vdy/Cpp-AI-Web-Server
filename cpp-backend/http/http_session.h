//
// Created by dongye on 25-5-3.
//

#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#pragma once
#include <string>
#include <chrono>
#include <iostream>

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../utils/utils.hpp"
#include "http_message.hpp"

constexpr int BUF_SIZE = 4096;

class HttpSession {
public:
    explicit HttpSession(FdWrapper&& fd) : fd_(std::move(fd)), last_active_(std::chrono::steady_clock::now()),
        body_read_complete_(false) {}

    HttpSession(const HttpSession&) = delete;
    HttpSession& operator=(const HttpSession&) = delete;

    // 支持移动
    HttpSession(HttpSession&& other) noexcept
      : fd_(std::move(other.fd_)),
        headerBuf_(std::move(other.headerBuf_)),
        last_active_(other.last_active_),
        body_read_complete_(other.body_read_complete_) {}

    HttpSession& operator=(HttpSession&& other) noexcept {
        if (this != &other) {
            fd_          = std::move(other.fd_);
            headerBuf_   = std::move(other.headerBuf_);
            last_active_ = other.last_active_;
            body_read_complete_ = other.body_read_complete_;
        }
        return *this;
    }

    bool read_header(HttpRequest& req);
    bool read_body(HttpRequest& req);
    const std::string get_header() const;

    void send_response(const std::string& data) const;

    void refresh_last_active();
    bool is_timed_out(int seconds) const;

private:
    FdWrapper fd_;
    std::string headerBuf_;
    std::chrono::steady_clock::time_point last_active_;
    bool body_read_complete_;

    ssize_t read_once(char* buf, size_t len) const;
    void parse(HttpRequest& req, const std::string& header);
};



#endif //HTTP_SESSION_H
