#pragma once

#include "curl_handle_manager.hpp"
#include "utils.hpp"
#include <atomic>
#include <curl/curl.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

class Mullvad {
private:
    const std::unordered_map<AccountStatus, std::string> statusFormats = {
        {AccountStatus::HIT, "[+] HIT | {} | "},
        {AccountStatus::CUSTOM, "[+] CUSTOM | {} | "},
        {AccountStatus::BAD, "[-] BAD | {} | "},
        {AccountStatus::ERROR, "[!] ERROR | {} | "}
    };

    const std::unordered_map<AccountStatus, fmt::text_style> statusStyles = {
        {AccountStatus::HIT, fg(fmt::color::green)},
        {AccountStatus::CUSTOM, fg(fmt::color::magenta)},
        {AccountStatus::BAD, fg(fmt::color::red)},
        {AccountStatus::ERROR, fg(fmt::color::red)}
    };

    std::mt19937 rng;
    CurlHandleManager curlManager;
    std::mutex fileMutex;
    std::mutex consoleMutex;
    std::atomic<int> hitCount{0};
    std::atomic<int> customCount{0};
    std::atomic<int> badCount{0};
    std::atomic<int> errorCount{0};
    int timeoutMs;
    std::string outputDir;

    std::string getRandomProxy(const std::vector<std::string>& proxies);
    bool createOutputDirectory();
    bool writeFile(const std::string& filename, const std::string& content);
    void print(const std::string& message);
    void logResult(const std::string& account, const std::string& capture, AccountStatus status);
    void handleResult(AccountStatus status, const std::string& accountNumber, const std::string& details = "");
    void printStats();

public:
    Mullvad(int timeout = DEFAULT_TIMEOUT_MS, const std::string& outputDirOverride = "");
    ~Mullvad();

    const std::string& getOutputDir() const;
    std::string generateAccountNumber();
    bool createAccount(const std::string& accountNumber);
    std::vector<std::string> readFile(const std::string& filename);
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems);
    void checkAccount(const std::string& accountNumber, const std::vector<std::string>& proxies);
    
    int getTotalHits() const;
    int getTotalCustom() const;
    int getTotalBad() const;
    int getTotalErrors() const;
};