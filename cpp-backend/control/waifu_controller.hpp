//
// Created by dongye on 25-5-6.
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

class WaifuController {
public:
    static HttpResponse random_prompts_handler(const HttpRequest& req) {
        HttpResponse response;
        try {
            std::string prompt(prompts_keywords_en);
            for (auto&& [k, v] : prompts_anime_elements_en) {
                if (k == "fantasy" or k == "light") {
                    if (rand_int(0, 2)) {
                        continue;
                    }
                }
                int i = rand_int(0, v.size() - 1);
                prompt += ", " + v[i];
            }
            json body;
            body["prompt"] = prompt;
            response.status = 200;
            response.body = body.dump();
        } catch (const std::exception& e) {
            response.status = 400;
        }
        return response;
    }

    static HttpResponse generate_handler(const HttpRequest& req) {
        HttpResponse response;
        std::unique_lock<std::mutex> lock(mtx_);
        try {
            auto j = json::parse(req.body);
            std::string prompt = j.at("prompt").get<std::string>();
            print_info(std::format("prompt = {}", prompt));
            auto&& _ = exec_predict(cmd + "'" + prompt + "'");
            fs::path newest;
            fs::file_time_type newestTime(fs::file_time_type::min());
            for (auto& entry : fs::directory_iterator(output_dir)) {
                if (!entry.is_regular_file()) continue;
                std::cout << fs::last_write_time(entry.path()) << " " << newestTime << std::endl;
                if (auto t = fs::last_write_time(entry.path()); t > newestTime) {
                    newest     = entry.path();
                    newestTime = t;
                }
            }
            json body;
            body["imageUrl"] = std::format("/figures/waifu/{}", newest.filename().string());
            response.status = 200;
            response.body = body.dump();
        } catch (const std::exception& e) {
            response.status = 400;
        }
        return response;
    }
private:
    inline static std::string cmd = "/home/dongye/anaconda3/envs/dy/bin/python /home/dongye/cpp_projects/clion-server/python_files/waifu.py --prompt ";
    inline static std::string prompts_keywords_en = "masterpiece, best quality, anime style, 1girl";
    inline static std::vector<std::string> colors = { "red", "blue", "green", "black", "white", "silver", "orange", "purple", "yellow" };
    inline static std::map<std::string, std::vector<std::string>> prompts_anime_elements_en = {
        {"eye", {"blue eyes", "green eyes", "red eyes", "golden eyes", "violet eyes", "heterochromia", "sparkling eyes", "black eyes"}},
        {"hair", {"long hair", "short hair", "twin tails", "ponytail", "braids", "silver hair", "pink hair", "gradient hair", "green hair", "black hair"}},
        {"outfits", {"school uniform", "sailor uniform", "cheongsam dress", "swimsuit", "kimono", "knight armor", "steampunk outfit"}},
        {"accessories", {"cat ears", "fox tail", "angel wings", "demon horns", "glasses", "earrings", "necklace"}},
        {"socks", {"thigh-high socks", "knee-high socks", "striped socks", "black stockings", "white stockings", "ankle socks"}},
        {"environments", {"cherry blossom background", "neon cityscape", "fantasy forest", "detailed background", "sunset sky", "rainy street", "ancient temple"}},
        {"fantasy", {"mecha", "magic", "floating crystals", "ethereal aura", "dragon"}},
        {"light", {"soft lighting", "dramatic shadows", "rim lighting", "volumetric light", "golden hour glow", "neon glow", "moonlight"}}
    };
    inline static std::string output_dir = "/home/dongye/cpp_projects/clion-server/web/figures/waifu/";
    inline static std::mutex mtx_;
    static std::string exec_predict(const std::string&& cmd) {
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
