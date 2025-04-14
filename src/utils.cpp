#include "../include/utils.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *s)
{
  size_t newLength = size * nmemb;
  try {
    s->append((char *)contents, newLength);
    return newLength;
  } catch (std::bad_alloc &e) {
    return 0;
  }
}

std::string strip(const std::string &str)
{
  if (str.empty())
    return str;
  auto startIt = std::find_if_not(
      str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
  if (startIt == str.end())
    return "";
  auto endIt = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
                 return std::isspace(c);
               }).base();
  return std::string(startIt, endIt);
}

std::string generateDateFolderName()
{
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "mullvad_results_";
  ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d_%H-%M-%S");
  return ss.str();
}

void clearScreen()
{
  fmt::print("\033[2J\033[1;1H");
}

std::string askInput(const std::string &msg)
{
  std::string input;
  fmt::print("{}", msg);
  std::getline(std::cin, input);
  return input;
}