//
// Created by dongye on 25-5-3.
//

#include "http_session.h"

bool HttpSession::read_header(HttpRequest& req) {
    char buf[BUF_SIZE];  // 默认header长度不超过4096
    ssize_t n = read_once(buf, sizeof(buf));
    if (n <= 0) return false;
    headerBuf_.append(buf, n);
    parse(req, headerBuf_);
    auto len = req.content_length();
    if (req.body.length() >= len) {
        body_read_complete_ = true;
    }
    refresh_last_active();
    return true;
}

bool HttpSession::read_body(HttpRequest& req) {
    if (body_read_complete_) {
        return true;
    }
    size_t len = req.content_length();
    if (len == 0) return true;

    char buf[BUF_SIZE << 1];
    while (req.body.length() < len) {
        size_t to_read = std::min(sizeof(buf), len - req.body.size());
        ssize_t n = read_once(buf, to_read);
        if (n <= 0) break;
        req.body.append(buf, n);
        if (req.body.size() >= len) {
            break;
        }
        refresh_last_active();
    }
    return req.body.size() >= len;
}

const std::string HttpSession::get_header() const {
    return headerBuf_;
}

void HttpSession::send_response(const std::string& data) const {
    const char* ptr = data.data();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t n = ::write(fd_.get(), ptr, remaining);
        if (n <= 0) break;
        ptr += n;
        remaining -= n;
    }
}

void HttpSession::refresh_last_active() {
    last_active_ = std::chrono::steady_clock::now();
}

bool HttpSession::is_timed_out(int seconds) const {
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - last_active_);
    return diff.count() > seconds;
}

ssize_t HttpSession::read_once(char* buf, size_t len) const {
    return ::read(fd_.get(), buf, len);
}

void HttpSession::parse(HttpRequest& req, const std::string& header) {
    int l = 0, r = header.find(' ');
    req.method = header.substr(l, r - l);
    l = r + 1, r = header.find(' ', l);
    req.path = header.substr(l, r - l);
    l = r + 1, r = header.find('\r', l);
    req.version = header.substr(l, r - l);
    std::cout << req.method << " " << req.path << " " << req.version << " !!!\n";
    const char* header_start = header.data() + r + 2;
    const char* p = header_start;
    const char* header_end = header.data() + header.length();
    [&]() {
        l = r = 0;
        std::string key, value;
        for (bool is_end = false;; p++) {
            char c = *p;
            switch (c)
            {
            case ':':
                key = std::string(header_start + l, header_start + r);
                std::cout << "[key]: " << key << ", ";
                r += 2, l = r, p++;
                break;
            case '\r':
                value = std::string(header_start + l, header_start + r);
                req.headers.emplace(key, value);
                std::cout << "[val]: " << value << "\n";
                r += 2, l = r, p++;
                if (is_end) {  // "\r\n\r\n", header end.
                    p++;
                    return;
                }
                is_end = true;
                break;
            default:
                is_end = false;
                r++;
                break;
            }
        }
    }();

    if (p != header_end) {
        req.body.assign(p, header_end);
    } else {
        body_read_complete_ = true;
    }
    std::cout << "[body]: " << req.body << "\n";
}
