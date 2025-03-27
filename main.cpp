#include <algorithm>
#include <bits/stdc++.h>
#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

class Mullvad {
private:
  int writeFile(std::string filename, std::string content)
  {
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

  std::string strip(std::string &in)
  {
    in.erase(
        std::remove_if(in.begin(), in.end(),
                       [](std::string::value_type ch) { return !isalpha(ch); }),
        in.end());
    return in;
  }

  std::vector<std::string> readFile(const std::string &filename)
  {
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

  void logger(int account, std::string capture, std::string status)
  {
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
      content = std::to_string(account) + " | " + capture;
    } else {
      filename = "errors.txt";
      content = "ERROR" + std::to_string(account);
    }
    writeFile(filename, content);
  }

  std::string gen(int amount)
  {
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

  void create(int amount)
  {
    std::string num = gen(amount);
    writeFile("created.txt", num);
  }

  void clear()
  {
    std::cout << "\033[2J\033[1;1H"; // Clear screen >.<
  }

  void checker(std::string accountNumber, std::vector<std::string> proxyArr)
  {
    std::string proxy = proxyArr[rand() % proxyArr.size()];
    std::string proxies[2]{{"http://" + proxy}, {"https://" + proxy}};
    std::cout << "Good: " << good << " ~ Custom: " << custom
              << " ~ Bad: " << bad << " ~ Errors:" << errors << std::endl;
    std::string url =
        "https://api-www.mullvad.net/www/accounts/" + strip(accountNumber);
  }
};

int main()
{
    return 0;
}
