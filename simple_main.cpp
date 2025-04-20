#include <iostream> 
#include "include/crawler.hpp" 
int main(int argc, char* argv[]) { 
    std::cout << "Web Crawler - Stub Implementation" << std::endl; 
    if (argc < 2) { 
        std::cout << "Usage: " << argv[0] << " ^<url^> [threads] [depth]" << std::endl; 
        return 1; 
    } 
    std::string url = argv[1]; 
    int threads = (argc > 2) ? std::stoi(argv[2]) : 4; 
    int depth = (argc > 3) ? std::stoi(argv[3]) : 3; 
    std::cout << "Would crawl: " << url << std::endl; 
    std::cout << "Threads: " << threads << std::endl; 
    std::cout << "Depth: " << depth << std::endl; 
    return 0; 
} 
