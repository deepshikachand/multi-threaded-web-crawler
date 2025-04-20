#pragma once

// Set C++ standard to C++17
#ifndef CPP_STANDARD
#define CPP_STANDARD 17
#endif

// Fix for curl.h not being found
#ifdef MINIMAL_BUILD
#define CURL_STATICLIB
// Define curl types and constants for minimal build without actual curl library
typedef void CURL;
typedef int CURLcode;
#define CURLE_OK 0
#define CURLOPT_URL 10000
#define CURLOPT_WRITEFUNCTION 20000
#define CURLOPT_WRITEDATA 10001
#define CURLOPT_XFERINFOFUNCTION 20001
#define CURLOPT_XFERINFODATA 10002
#define CURLOPT_USERAGENT 10004
#define CURLOPT_TIMEOUT 13
#define CURLOPT_FOLLOWLOCATION 52
#define CURLINFO_RESPONSE_CODE 2097154
#define CURLINFO_CONTENT_TYPE 1048594

// Stub declarations for curl functions
inline CURL* curl_easy_init() { return nullptr; }
inline CURLcode curl_easy_setopt(CURL*, int, ...) { return CURLE_OK; }
inline CURLcode curl_easy_perform(CURL*) { return CURLE_OK; }
inline CURLcode curl_easy_getinfo(CURL*, int, ...) { return CURLE_OK; }
inline void curl_easy_cleanup(CURL*) {}
inline const char* curl_easy_strerror(CURLcode) { return "No error"; }
#endif

// Fix for missing nlohmann/json.hpp
#ifdef MINIMAL_BUILD
#include <string>
#include <vector>
#include <map>

namespace nlohmann {
    class json {
    public:
        json() {}
        json(std::nullptr_t) {}
        
        template<typename T>
        json(const T&) {}
        
        bool contains(const std::string&) const { return false; }
        
        template<typename T>
        T value(const std::string&, const T& defaultValue) const { return defaultValue; }
        
        template<typename T>
        operator T() const { return T(); }
        
        json& operator[](const std::string&) { return *this; }
        
        static json parse(const std::string&) { return json(); }
        std::string dump(int = 2) const { return "{}"; }
    };
}
#endif 