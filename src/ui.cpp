#include "../include/ui.hpp"
#include "../include/mullvad.hpp"
#include "../include/thread_safe_queue.hpp"
#include "../include/utils.hpp"
#include <atomic>
#include <fmt/color.h>
#include <string>
#include <thread>
#include <vector>

void workerThread(Mullvad &mullvad, ThreadSafeQueue<std::string> &accountQueue,
                  const std::vector<std::string> &proxies,
                  std::atomic<bool> &done)
{
  std::string accountNumber;
  while (!done || !accountQueue.empty()) {
    if (accountQueue.pop(accountNumber)) {
      mullvad.checkAccount(accountNumber, proxies);
    } else {
      if (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
  }
}

void showMenu()
{
  std::vector<std::string> op = {"Generate accounts", "Check accounts",
                                 "Generate and check generated accounts",
                                 "Quit"};
  fmt::print("Mullvad Account Checker/Generator\n");
  for (size_t i = 0; i < op.size(); ++i) {
    fmt::print(fmt::format("{}. {}\n", i + 1, op[i]));
  }
  fmt::print("--------------------------------\n");
}

int menu()
{
  Mullvad mullvad;
  while (true) {
    clearScreen();
    showMenu();
    std::string op = askInput("[?] Insert option: ");
    try {
      int choice = std::stoi(op);
      switch (choice) {
      case 1: {
        int generateCount = std::stoi(askInput("[?] How much: "));
        for (int i = 0; i < generateCount; i++) {
          std::string accountNumber = mullvad.generateAccountNumber();
          mullvad.createAccount(accountNumber);
          if (generateCount > 100 && i % 100 == 0 && i > 0) {
            fmt::print("[*] Generated and queued {}/{} accounts\n", i,
                       generateCount);
          }
        }
        askInput("[*] Press Enter to continue...");
        break;
      }
      case 2: {
        std::string accountPath =
            askInput("[?] Insert the path of the accounts to check: ");
        auto accounts = mullvad.readFile(accountPath);
        if (accounts.empty()) {
          fmt::print("[!] No accounts found in {}\n", accountPath);
          askInput("[*] Press Enter to continue...");
          break;
        }
        std::string proxyPath =
            askInput("[?] Insert the path to proxies file: ");
        auto proxies = mullvad.readFile(proxyPath);
        if (proxies.empty()) {
          fmt::print("[!] No proxies found in {}\n", proxyPath);
          askInput("[*] Press Enter to continue...");
          break;
        }
        int threadCount = std::stoi(askInput("[?] Insert number of threads: "));
        threadCount = std::min(std::max(threadCount, 1), MAX_THREADS);
        std::vector<std::thread> threads;
        ThreadSafeQueue<std::string> accountQueue;
        std::atomic<bool> done{false};
        for (const auto &account : accounts) {
          accountQueue.push(account);
        }
        for (int i = 0; i < threadCount; ++i) {
          threads.emplace_back(workerThread, std::ref(mullvad),
                               std::ref(accountQueue), std::ref(proxies),
                               std::ref(done));
        }
        for (auto &thread : threads) {
          thread.join();
        }
        askInput("[*] Press Enter to continue...");
        break;
      }
      case 3: {
        int generateCount =
            std::stoi(askInput("[?] How much accounts to generate: "));
        std::string proxyPath =
            askInput("[?] Insert the path to proxies file: ");
        auto proxies = mullvad.readFile(proxyPath);
        if (proxies.empty()) {
          fmt::print("[!] No proxies found in {}\n", proxyPath);
          askInput("[*] Press Enter to continue...");
          break;
        }
        int threadCount = std::stoi(askInput("[?] Insert number of threads: "));
        threadCount = std::min(std::max(threadCount, 1), MAX_THREADS);
        std::vector<std::thread> threads;
        ThreadSafeQueue<std::string> accountQueue;
        std::atomic<bool> done{false};
        for (int i = 0; i < generateCount; i++) {
          std::string accountNumber = mullvad.generateAccountNumber();
          mullvad.createAccount(accountNumber);
          accountQueue.push(accountNumber);
          if (generateCount > 100 && i % 100 == 0 && i > 0) {
            fmt::print("[*] Generated and queued {}/{} accounts\n", i,
                       generateCount);
          }
        }
        for (int i = 0; i < threadCount; ++i) {
          threads.emplace_back(workerThread, std::ref(mullvad),
                               std::ref(accountQueue), std::ref(proxies),
                               std::ref(done));
        }
        for (auto &thread : threads) {
          thread.join();
        }
        askInput("[*] Press Enter to continue...");
        break;
      }
      case 4:
        return 0;
      default:
        fmt::print("[!] Invalid option\n");
        askInput("[*] Press Enter to continue...");
      }
    } catch (const std::exception &e) {
      fmt::print("[!] Error: {}\n", e.what());
      askInput("[*] Press Enter to continue...");
    }
  }
  return 0;
}