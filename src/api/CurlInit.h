#pragma once
#include <curl/curl.h>

struct CurlInit {
    CurlInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlInit() { curl_global_cleanup(); }
    CurlInit(const CurlInit&) = delete;
    CurlInit& operator=(const CurlInit&) = delete;
};
