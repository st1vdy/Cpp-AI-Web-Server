//
// Created by dongye on 25-5-3.
//
#pragma once
#include <format>
#include <chrono>
#include <iostream>

inline void print_info(std::string&& info) {
    std::cout << std::format("[INFO] {:%Y-%m-%d %H:%M:%S}.\n", std::chrono::system_clock::now());
    std::cout << info << std::endl;
}