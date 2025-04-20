#pragma once

#include <string>
#include <vector>
#include <memory>
#include <limits>
#include <sstream>
// Comment out problematic includes for now
// #include <torch/torch.h>
// #include <torch/script.h>

class ContentAnalyzer {
public:
    struct ContentFeatures {
        std::string language;
        std::vector<std::string> topics;
        double relevance;
        bool isSpam;
        std::vector<std::string> entities;

        // Add a method to convert ContentFeatures to string
        std::string toString() const {
            std::stringstream ss;
            ss << "Language: " << language << ", Topics: ";
            for (const auto& topic : topics) ss << topic << ", ";
            ss << "Relevance: " << relevance << ", IsSpam: " << isSpam;
            return ss.str();
        }
    };

    ContentAnalyzer();
    ~ContentAnalyzer();

    // Content analysis
    ContentFeatures analyzeContent(const std::string& content);
    std::vector<std::string> extractKeywords(const std::string& content);
    double calculateRelevance(const std::string& content, const std::string& query);
    bool isSpam(const std::string& content);
    std::vector<std::string> extractEntities(const std::string& content);

    // Model management
    void loadModels(const std::string& modelDir);
    void saveModels(const std::string& modelDir);

private:
    // ML models - using forward declarations for now
    class TorchModule; // Forward declaration
    std::unique_ptr<TorchModule> languageModel;
    std::unique_ptr<TorchModule> topicModel;
    std::unique_ptr<TorchModule> spamModel;
    std::unique_ptr<TorchModule> entityModel;

    // Text preprocessing
    std::string preprocessText(const std::string& text);
    std::vector<float> tokenize(const std::string& text);
    
    // Model inference
    std::string detectLanguage(const std::string& content);
    std::vector<std::string> classifyTopics(const std::string& content);
    bool detectSpam(const std::string& content);
    std::vector<std::string> extractNamedEntities(const std::string& content);
}; 