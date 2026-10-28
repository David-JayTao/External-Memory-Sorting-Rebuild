#pragma once
#include "external/run_file.h"
#include <iostream>
#include <stdexcept>
#include <string>

inline void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Action>
void expect_error(Action action) {
    bool caught = false;
    try { action(); } catch (const std::exception&) { caught = true; }
    check(caught, "Expected exception");
}
// CTest 为各周提供独立工作目录，只清理由测试自己建立的子目录。
class TestDirectory {
public:
    TestDirectory() {
        check(std::filesystem::create_directory(path), "Test scratch directory already exists");
    }
    ~TestDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    const std::filesystem::path path = "scratch";
};
