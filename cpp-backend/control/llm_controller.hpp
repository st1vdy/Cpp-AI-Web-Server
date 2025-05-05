//
// Created by dongye on 25-5-4.
//
#pragma once
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../utils/utils.hpp"
#include "../http/http_message.hpp"
#include "../thread/thread_pool.hpp"

using json = nlohmann::json;

class ChatManager {
public:
    static ChatManager& instance() {
        static ChatManager inst;
        return inst;
    }

    std::string handle(const std::string& token, const std::string& message) {
        std::lock_guard<std::mutex> lock(mu_);

        auto& history = sessions_[token];
        history.push_back({ {"role", "user"}, {"content", message} });

        json req_body = {
            {"model", "deepseek-r1:14b"},
            {"messages", history}
        };
        std::string req_str = req_body.dump();
        std::string resp_str = post_to_ollama(req_str);
        json resp_json = json::parse(resp_str);
        std::string reply = resp_json["choices"][0]["message"]["content"];

        history.push_back({ {"role", "assistant"}, {"content", reply} });
        return reply;
    }

private:
    ChatManager() {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    ~ChatManager() {
        curl_global_cleanup();
    }
    ChatManager(const ChatManager&) = delete;
    ChatManager& operator=(const ChatManager&) = delete;

    std::string post_to_ollama(const std::string& body) {
        CURL* curl = curl_easy_init();
        std::string response, header;
        if (!curl) return "";

        curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:11434/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

        struct curl_slist* hdrs = nullptr;
        hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                auto& resp = *static_cast<std::string*>(userdata);
                size_t total = size * nmemb;
                resp.append(ptr, total);
                return total;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(hdrs);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK) {
            return "";
        }
        return response;
    }

    std::mutex mu_;
    std::unordered_map<std::string, std::vector<json>> sessions_;
};

inline std::pair<std::string, std::string> split_think(std::string& s) {
    const std::string open_tag  = "<think>";
    const std::string close_tag = "</think>";

    auto pos_open  = s.find(open_tag);
    auto pos_close = s.find(close_tag, pos_open + open_tag.size());

    if (pos_open == std::string_view::npos ||
        pos_close == std::string_view::npos ||
        pos_close < pos_open + open_tag.size()) {
        return { {}, {} };
    }

    auto&& content = s.substr(pos_open + open_tag.size(), pos_close - (pos_open + open_tag.size()));
    auto&& tail = s.substr(pos_close + close_tag.size());
    content = trim(content);
    tail = trim(tail);

    return { content, tail };
}

class LLMController {
public:
    static HttpResponse chat_handle(const HttpRequest& req) {
        HttpResponse response;
        try {
            response.status = 200;
            auto j = json::parse(req.body);
            std::string message = j["message"].get<std::string>();
            std::string token = j["token"].get<std::string>();
            print_info(std::format("Client {}, message: {}", token, message));
            auto&& reply = ChatManager::instance().handle(token, message);
            auto&& [think_str, reply_str] = split_think(reply);
            print_info(std::format("Think: {}", think_str));
            print_info(std::format("Reply: {}", reply_str));

            json body;
            body["think"] = think_str;
            body["reply"] = reply_str;
            response.body = body.dump();
        } catch (...) {
            response.status = 400;
            response.reason = "Bad Request";
            json body = {{"message", "请求格式错误"}};
            response.body = body.dump();
        }
        return response;
    }
};