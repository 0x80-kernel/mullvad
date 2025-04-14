#pragma once

#include <curl/curl.h>
#include <vector>
#include <mutex>

class CurlHandleManager {
private:
    std::vector<CURL*> handles;
    std::mutex handleMutex;

public:
    CurlHandleManager(size_t initialPoolSize = 20) {
        std::lock_guard<std::mutex> lock(handleMutex);
        for (size_t i = 0; i < initialPoolSize; ++i) {
            CURL* curl = curl_easy_init();
            if (curl) {
                handles.push_back(curl);
            }
        }
    }

    ~CurlHandleManager() {
        std::lock_guard<std::mutex> lock(handleMutex);
        for (auto handle : handles) {
            curl_easy_cleanup(handle);
        }
    }

    CURL* getHandle() {
        std::lock_guard<std::mutex> lock(handleMutex);
        if (handles.empty()) {
            return curl_easy_init();
        } else {
            CURL* handle = handles.back();
            handles.pop_back();
            curl_easy_reset(handle);
            return handle;
        }
    }

    void releaseHandle(CURL* handle) {
        if (handle) {
            std::lock_guard<std::mutex> lock(handleMutex);
            handles.push_back(handle);
        }
    }
};