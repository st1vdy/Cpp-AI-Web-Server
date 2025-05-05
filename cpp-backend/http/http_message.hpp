//
// Created by dongye on 25-5-3.
//
#pragma once
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

#include "../log/logger.hpp"
#include "../utils/utils.hpp"

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    size_t content_length() {
        if (not this->headers.contains("Content-Length")) {
            return 0;
        }
        return stoul(headers["Content-Length"]);
    }
};


struct HttpResponse {
    std::string version = "HTTP/1.1";
    int status = 200;
    std::string reason = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    std::string to_response_string() const {
        std::ostringstream out;
        out << version << " " << status;
        if (!reason.empty()) out << " " << reason;
        out << "\r\n";
        for (const auto& [key, val] : headers) {
            out << key << ": " << val << "\r\n";
        }
        out << "\r\n";
        out << body;
        return out.str();
    }
};