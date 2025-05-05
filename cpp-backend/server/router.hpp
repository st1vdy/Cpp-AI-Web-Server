//
// Created by dongye on 25-5-3.
//

#pragma once
#include <tuple>
#include <vector>
#include <string>
#include <functional>
#include <array>

#include "../http/http_session.h"
#include "../http/http_message.hpp"

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class trie {
public:
    explicit trie(int size = 256) : cnt(0), nxt(size), vis(size, 0) {}
    void insert(const std::string& s, Handler&& handle) {
        int p = 0;
        for (int i = 0; i < s.length(); i++) {
            int c = s[i];
            if (!nxt[p][c]) nxt[p][c] = ++cnt;
            p = nxt[p][c];
        }
        handles.emplace_back(handle);
        vis[p] = handles.size();
    }
    std::optional<Handler> find(const std::string& s) const {
        int p = 0;
        for (int i = 0; i < s.length(); i++) {
            int c = s[i];
            if (!nxt[p][c]) return std::nullopt;
            p = nxt[p][c];
        }
        if (!vis[p]) return std::nullopt;
        return handles[vis[p] - 1];
    }
private:
    int cnt;
    std::vector<std::array<int, 128>> nxt;
    std::vector<int> vis;
    std::vector<Handler> handles;
};

class Router {
public:
    void register_route(const std::string& method, const std::string& path, Handler handler) {
        std::string str = method + path;
        trie_.insert(str, std::move(handler));
    }
    HttpResponse route(const HttpRequest& req) const {
        std::cout << "[INFO]: method: " << req.method << " path: " << req.path << std::endl;
        std::string str = req.method + req.path;
        if (auto opt_handler = trie_.find(str)) {
            auto& handler = *opt_handler;
            return handler(req);
        }
        // 未找到匹配，返回 404
        HttpResponse resp;
        resp.status = 404;
        resp.reason = "Not Found";
        resp.body   = R"({"message":"Not Found"})";
        resp.headers["Content-Type"] = "application/json";
        return resp;
    }
private:
    trie trie_;
};