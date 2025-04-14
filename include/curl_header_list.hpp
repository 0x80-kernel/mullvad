#pragma once

#include <curl/curl.h>
#include <string>

class CurlHeaderList {
private:
    struct curl_slist* headers = nullptr;

public:
    CurlHeaderList() = default;

    ~CurlHeaderList() {
        if (headers) {
            curl_slist_free_all(headers);
            headers = nullptr;
        }
    }

    CurlHeaderList(const CurlHeaderList&) = delete;
    CurlHeaderList& operator=(const CurlHeaderList&) = delete;

    CurlHeaderList(CurlHeaderList&& other) noexcept : headers(other.headers) {
        other.headers = nullptr;
    }

    CurlHeaderList& operator=(CurlHeaderList&& other) noexcept {
        if (this != &other) {
            if (headers) {
                curl_slist_free_all(headers);
            }
            headers = other.headers;
            other.headers = nullptr;
        }
        return *this;
    }

    void append(const std::string& header) {
        headers = curl_slist_append(headers, header.c_str());
    }

    struct curl_slist* get() const { return headers; }
};