#pragma once

#ifdef __INTELLISENSE__
// Only include this file for IntelliSense

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <queue>
#include <thread>
#include <chrono>
#include <regex>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// This file mainly exists to help IntelliSense recognize all the WebCrawler methods
// The actual definitions should be in crawler.hpp which is included in the actual build

// Only define these if not already defined
#ifndef CURL_DEFINED
#define CURL_DEFINED
// Forward declarations of CURL types
typedef void CURL;
typedef long CURLcode;
typedef long curl_off_t;

// CURL constants
#define CURLOPT_URL 1
#define CURLOPT_WRITEFUNCTION 2
#define CURLOPT_WRITEDATA 3
#define CURLOPT_XFERINFOFUNCTION 4
#define CURLOPT_XFERINFODATA 5
#define CURLOPT_HEADER 6
#define CURLOPT_NOBODY 7
#define CURLE_OK 0
#define CURLINFO_RESPONSE_CODE 100
#define CURLINFO_CONTENT_TYPE 101

// CURL stub functions - use inline to avoid multiple definition errors
inline CURL* curl_easy_init() { return nullptr; }
inline void curl_easy_cleanup(CURL* handle) {}
inline CURLcode curl_easy_setopt(CURL* handle, int option, ...) { return 0; }
inline CURLcode curl_easy_perform(CURL* handle) { return 0; }
inline CURLcode curl_easy_getinfo(CURL* handle, int info, ...) { return 0; }
inline const char* curl_easy_strerror(CURLcode code) { return ""; }
#endif // CURL_DEFINED

#endif // __INTELLISENSE__ 