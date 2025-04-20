#include "../include/curl_stubs.hpp"

// Define stub implementations for CURL functions regardless of STUB_IMPLEMENTATION

CURL* curl_easy_init() { 
    return (CURL*)1; // Return non-null pointer
}

void curl_easy_cleanup(CURL* handle) { 
    // Do nothing
}

CURLcode curl_easy_setopt(CURL* handle, int option, ...) {
    return 0;
}

CURLcode curl_easy_perform(CURL* handle) {
    return 0;
}

CURLcode curl_easy_getinfo(CURL* handle, int info, ...) {
    return 0;
}

const char* curl_easy_strerror(CURLcode code) {
    return "Stub error";
}

char* curl_easy_escape(void* handle, const char* string, int length) {
    return nullptr;
}

void curl_free(void* ptr) {
    // Do nothing
}

char* curl_easy_unescape(void* handle, const char* url, int length, int* outlength) {
    return nullptr;
} 