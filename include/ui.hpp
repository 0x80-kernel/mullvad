#ifndef UI_HPP
#define UI_HPP

#include "mullvad.hpp"
#include "thread_safe_queue.hpp"
#include <atomic>
#include <string>
#include <vector>

void showMenu();
int menu();
void workerThread(Mullvad &mullvad, ThreadSafeQueue<std::string> &accountQueue,
                  const std::vector<std::string> &proxies,
                  std::atomic<bool> &done);

#endif // UI_HPP
