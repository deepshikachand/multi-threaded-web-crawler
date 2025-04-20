#pragma once

#ifdef __INTELLISENSE__

// This file provides declarations for thread_pool.cpp for IntelliSense support

#include <vector>

// Define the missing ThreadPoolAttributes struct 
struct ThreadPoolAttributes {
    size_t thread_index;
    unsigned long long processor_mask;
};

// Vector of thread attributes that is used in thread_pool.cpp
std::vector<ThreadPoolAttributes> thread_attributes;

#endif // __INTELLISENSE__ 