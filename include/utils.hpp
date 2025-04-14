#pragma once

#include <string>
#include <fmt/format.h>

constexpr int DEFAULT_TIMEOUT_MS = 5000;
constexpr int MAX_THREADS = 10;

enum class AccountStatus { HIT, CUSTOM, BAD, ERROR };

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
std::string strip(const std::string& str);
std::string generateDateFolderName();
void clearScreen();
std::string askInput(const std::string& msg);