//
// Created by dongye on 25-5-4.
//
#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <fstream>
#include <optional>
#include <filesystem>

#include "../utils/utils.hpp"
#include "../http/http_message.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

inline void save_image(const std::string& image_data, const std::string& filepath) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        throw std::runtime_error("无法打开文件进行写入: " + filepath);
    }
    out.write(image_data.data(), image_data.size());
    out.close();
}

inline std::string extract_boundary(const std::string& ct) {
    auto p = ct.find("boundary=");
    auto&& v = ct.substr(p + 9);
    while (v.back() == '\r' or v.back() == '\n') {
        v.pop_back();
    }
    return v;
}

inline std::pair<std::string, std::string> parse_multipart_file(const std::string& body, const std::string& boundary) {
    std::string d = "--" + boundary;
    auto pos = body.find(d);
    while (pos != std::string::npos) {
        pos += d.size();
        if (body.substr(pos, 2) == "--") break;
        pos = body.find("\r\n\r\n", pos);
        if (pos == std::string::npos) break;
        auto hdr = body.substr(0, pos);
        auto f = hdr.find("filename=\"");
        if (f != std::string::npos) {
            f += 10; auto q = hdr.find('"', f);
            auto name = hdr.substr(f, q - f);
            auto ds = pos + 4;
            auto ne = body.find(d, ds);
            auto de = (ne != std::string::npos && body[ne - 2] == '\r' && body[ne - 1] == '\n' ? ne - 2 : ne);
            return std::make_pair(name, body.substr(ds, de - ds));
        }
        pos = body.find(d, pos);
    }
    return {};
}

class ScenimefyController {
public:
    static HttpResponse sample_handle(const HttpRequest& req) {
        HttpResponse response;
        try {
            int id = rand_int(0, 19);
            json body;
            std::string original_sample_url(std::format("/figures/scenimefy_samples/a/{}.jpg", id));
            std::string stylized_sample_url(std::format("/figures/scenimefy_samples/b/{}.png", id));
            body["originalSampleUrl"] = original_sample_url;
            body["stylizedSampleUrl"] = stylized_sample_url;
            print_info(std::format("Random sample for Scenimefy: originalSampleUrl: {}; stylizedSampleUrl: {}",
                original_sample_url, stylized_sample_url));
            response.status = 200;
            response.body = body.dump();
        } catch (const std::exception& e) {
            response.status = 400;
        }
        return response;
    }

    static HttpResponse scenimefy_handle(const HttpRequest& req) {
        HttpResponse response;
        std::unique_lock<std::mutex> lock(mtx_);
        for (auto& entry : fs::directory_iterator(test_dir)) {  // 先清空该目录 该类只允许一个线程同时进行操作
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
            }
        }
        auto it = req.headers.find("Content-Type");
        for (auto&& [x, y] : req.headers) {
            std::cout << x << ": " << y << std::endl;
        }
        std::string boundary(it->second);
        boundary = extract_boundary(boundary);
        auto&& [filename, data] = parse_multipart_file(req.body, boundary);
        try {
            save_image(data, test_dir + filename);
            auto&& _ = exec_predict(cmd);
            fs::path newest;
            fs::file_time_type newestTime(fs::file_time_type::min());
            for (auto& entry : fs::directory_iterator(output_dir)) {
                std::cout << entry.path() << std::endl;
                if (!entry.is_regular_file()) continue;
                std::cout << fs::last_write_time(entry.path()) << " " << newestTime << std::endl;
                if (auto t = fs::last_write_time(entry.path()); t > newestTime) {
                    newest     = entry.path();
                    newestTime = t;
                }
            }
            print_info(std::format("Writing to {}...", newest.string()));
            json body;
            std::string stylized_image_url(std::format("/figures/scenimefy_outputs/shinkai-test/test_Shinkai/images/fake_B/{}", newest.filename().string()));;
            body["stylizedImageUrl"] = stylized_image_url;
            response.body = body.dump();
            response.status = 200;
        } catch (const std::exception& e) {
            response.status = 400;
        }
        return response;
    }
private:
    inline static std::mutex mtx_;
    inline static std::string test_dir = "/data0/dy/Scenimefy/Semi_translation/datasets/Sample/testA/";
    inline static std::string cmd = "/home/dongye/anaconda3/envs/dy/bin/python /data0/dy/Scenimefy/Semi_translation/test.py "
                  "--dataroot /data0/dy/Scenimefy/Semi_translation/datasets/Sample --name shinkai-test "
                  "--CUT_mode CUT  --model cut --phase test --epoch Shinkai --preprocess none"
                  " --results_dir /home/dongye/cpp_projects/clion-server/web/figures/scenimefy_outputs"
                  " --checkpoints_dir /data0/dy/Scenimefy/Semi_translation/pretrained_models/";
    inline static std::string output_dir = "/home/dongye/cpp_projects/clion-server/web/figures/scenimefy_outputs/shinkai-test/test_Shinkai/images/fake_B";

    static std::string exec_predict(const std::string& cmd) {
        std::vector<char> buffer(4096);
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        std::size_t nread = 0;
        while ((nread = std::fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) {
            result.append(buffer.data(), nread);
        }
        return result;
    }
};