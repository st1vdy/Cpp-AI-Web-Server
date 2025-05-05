//
// Created by dongye on 25-5-3.
//

#pragma once
#include <mutex>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "../http/http_message.hpp"
#include "../utils/utils.hpp"

inline bool check_password_security(const std::string& password) {
    return password.length() <= 5;
}

class LoginController {
public:
    using json = nlohmann::json;
    static HttpResponse login_handle(const HttpRequest& req) {
        HttpResponse response;
        try {
            auto j = json::parse(req.body);
            std::string email    = j.at("email").get<std::string>();
            std::string password = j.at("password").get<std::string>();  // TODO: 密码应该hash
            print_info(std::format("User: {} trying to login, password: {}", email, password));
            std::unique_lock<std::mutex> lock(mtx_);
            if (auto it = user_db_.find(email); it != user_db_.end() && it->second == password) {
                // 登录成功
                print_info(std::format("User: {} login successfully!", email));
                response.status = 200;
                response.reason = "OK";
                json body = {
                    {"message", "登陆成功"},
                    {"token", email}
                };
                response.body = body.dump();
            } else {
                // 用户不存在或密码错误
                print_info(std::format("User: {} login with WRONG password.", email));
                response.status = 401;
                response.reason = "Unauthorized";
                json body = {{"message", "邮箱或密码错误"}};
                response.body = body.dump();
            }
        } catch (json::exception& e) {
            response.status = 400;
            response.reason = "Bad Request";
            json body = {{"message", "请求格式错误"}, {"error", e.what()}};
            response.body = body.dump();
        }
        return response;
    }

    static HttpResponse register_handle(const HttpRequest& req) {
        HttpResponse response;
        try {
            auto j = json::parse(req.body);
            std::string email    = j.at("email").get<std::string>();
            std::string password = j.at("password").get<std::string>();  // TODO: 密码应该hash
            print_info(std::format("User: {} trying to register, password: {}", email, password));
            std::unique_lock<std::mutex> lock(mtx_);
            if (auto it = user_db_.find(email); it != user_db_.end()) {
                print_info(std::format("User: {} already registered.", email));
                response.status = 401;
                response.reason = "Duplicate user";
                json body = {{"message", "用户名已存在"}};
                response.body = body.dump();
            } else if (check_password_security(password)) {
                print_info(std::format("User: {}'s pw too simple.", email));
                response.status = 401;
                response.reason = "PW too simple";
                json body = {{"message", "密码过于简单(长度<6位)"}};
                response.body = body.dump();
            } else {
                user_db_.emplace(email, password);
                lock.unlock();
                print_info(std::format("User: {} register successfully!", email));
                response.status = 200;
                response.reason = "OK";
                json body = {{"message", "注册成功"}};
                response.body = body.dump();
            }
        } catch (json::exception& e) {
            response.status = 400;
            response.reason = "Bad Request";
            json body = {{"message", "请求格式错误"}, {"error", e.what()}};
            response.body = body.dump();
        }
        return response;
    }

    static HttpResponse check_auth_token(const HttpRequest& req) {
        HttpResponse response;
        try {
            auto it = req.headers.find("Cookie");
            auto&& auth_token = it->second.substr(it->second.find('=') + 1);
            print_info(std::format("Checking auth_token: [{}]...", auth_token));
            std::unique_lock<std::mutex> lock(mtx_);
            if (user_db_.contains(auth_token)) {
                print_info(std::format("Auth_token: {} exist, confirm request!", auth_token));
                response.status = 200;
            } else {
                print_info(std::format("Auth_token: {} doesn't exist.", auth_token));
                response.status = 401;
            }
        } catch (json::exception& e) {
            print_info(std::format("Auth_token check error."));
            response.status = 400;
        }
        return response;
    }
private:
    inline static std::mutex mtx_;
    inline static std::map<std::string, std::string> user_db_ = {
        {"22351056@zju.edu.cn", "114514"},
    };
};
