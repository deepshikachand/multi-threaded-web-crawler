#pragma once

#include <string>
#include <vector>
#include <memory>
#include <torch/torch.h>
#include <torch/script.h>

class ContentAnalyzer {
public:
    struct ContentFeatures {
        std::string language;
        std::vector<std::string> topics;
        double relevance;
        bool isSpam;
        std::vector<std::string> entities;
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
    // ML models
    torch::jit::Module languageModel;
    torch::jit::Module topicModel;
    torch::jit::Module spamModel;
    torch::jit::Module entityModel;

    // Text preprocessing
    std::string preprocessText(const std::string& text);
    std::vector<float> tokenize(const std::string& text);
    
    // Model inference
    std::string detectLanguage(const std::string& content);
    std::vector<std::string> classifyTopics(const std::string& content);
    bool detectSpam(const std::string& content);
    std::vector<std::string> extractNamedEntities(const std::string& content);
}; 