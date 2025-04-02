#include "content_analyzer.hpp"
#include <stdexcept>
#include <algorithm>
#include <regex>
#include <fstream>
#include <sstream>

ContentAnalyzer::ContentAnalyzer() {
    // Initialize PyTorch models
    try {
        loadModels("models");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize ML models: " + std::string(e.what()));
    }
}

ContentAnalyzer::~ContentAnalyzer() {
    // Cleanup is handled by PyTorch
}

ContentAnalyzer::ContentFeatures ContentAnalyzer::analyzeContent(const std::string& content) {
    ContentFeatures features;
    
    // Preprocess content
    std::string processed = preprocessText(content);
    
    // Run analysis
    features.language = detectLanguage(processed);
    features.topics = classifyTopics(processed);
    features.isSpam = detectSpam(processed);
    features.entities = extractNamedEntities(processed);
    
    return features;
}

std::vector<std::string> ContentAnalyzer::extractKeywords(const std::string& content) {
    std::string processed = preprocessText(content);
    auto tokens = tokenize(processed);
    
    // Create input tensor
    torch::Tensor input = torch::from_blob(tokens.data(), 
        {(long)tokens.size()}, torch::kFloat32);
    
    // Run inference
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = topicModel.forward(inputs).toTuple();
    
    // Process output
    auto scores = output->elements()[0].toTensor();
    auto indices = std::get<0>(torch::topk(scores, 5));
    
    // Convert to keywords
    std::vector<std::string> keywords;
    for (int i = 0; i < indices.size(0); i++) {
        keywords.push_back(std::to_string(indices[i].item<int>()));
    }
    
    return keywords;
}

double ContentAnalyzer::calculateRelevance(const std::string& content, const std::string& query) {
    std::string processedContent = preprocessText(content);
    std::string processedQuery = preprocessText(query);
    
    auto contentTokens = tokenize(processedContent);
    auto queryTokens = tokenize(processedQuery);
    
    // Create input tensors
    torch::Tensor contentTensor = torch::from_blob(contentTokens.data(),
        {(long)contentTokens.size()}, torch::kFloat32);
    torch::Tensor queryTensor = torch::from_blob(queryTokens.data(),
        {(long)queryTokens.size()}, torch::kFloat32);
    
    // Run similarity calculation
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(contentTensor);
    inputs.push_back(queryTensor);
    auto output = topicModel.forward(inputs).toTensor();
    
    return output.item<double>();
}

bool ContentAnalyzer::isSpam(const std::string& content) {
    std::string processed = preprocessText(content);
    auto tokens = tokenize(processed);
    
    // Create input tensor
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    // Run spam detection
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = spamModel.forward(inputs).toTensor();
    
    return output.item<double>() > 0.5;
}

std::vector<std::string> ContentAnalyzer::extractEntities(const std::string& content) {
    std::string processed = preprocessText(content);
    auto tokens = tokenize(processed);
    
    // Create input tensor
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    // Run NER
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = entityModel.forward(inputs).toTensor();
    
    // Process output
    std::vector<std::string> entities;
    auto indices = std::get<0>(torch::nonzero(output > 0.5));
    
    for (int i = 0; i < indices.size(0); i++) {
        entities.push_back(std::to_string(indices[i].item<int>()));
    }
    
    return entities;
}

void ContentAnalyzer::loadModels(const std::string& modelDir) {
    try {
        languageModel = torch::jit::load(modelDir + "/language_model.pt");
        topicModel = torch::jit::load(modelDir + "/topic_model.pt");
        spamModel = torch::jit::load(modelDir + "/spam_model.pt");
        entityModel = torch::jit::load(modelDir + "/entity_model.pt");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load models: " + std::string(e.what()));
    }
}

void ContentAnalyzer::saveModels(const std::string& modelDir) {
    try {
        languageModel.save(modelDir + "/language_model.pt");
        topicModel.save(modelDir + "/topic_model.pt");
        spamModel.save(modelDir + "/spam_model.pt");
        entityModel.save(modelDir + "/entity_model.pt");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save models: " + std::string(e.what()));
    }
}

std::string ContentAnalyzer::preprocessText(const std::string& text) {
    // Convert to lowercase
    std::string processed = text;
    std::transform(processed.begin(), processed.end(), processed.begin(), ::tolower);
    
    // Remove HTML tags
    std::regex htmlRegex("<[^>]*>");
    processed = std::regex_replace(processed, htmlRegex, "");
    
    // Remove special characters
    std::regex specialRegex("[^a-z0-9\\s]");
    processed = std::regex_replace(processed, specialRegex, " ");
    
    // Remove extra whitespace
    std::regex whitespaceRegex("\\s+");
    processed = std::regex_replace(processed, whitespaceRegex, " ");
    
    return processed;
}

std::vector<float> ContentAnalyzer::tokenize(const std::string& text) {
    // Simple word-based tokenization
    std::vector<float> tokens;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Simple hash-based tokenization
        size_t hash = std::hash<std::string>{}(word);
        tokens.push_back(static_cast<float>(hash) / std::numeric_limits<size_t>::max());
    }
    
    return tokens;
}

std::string ContentAnalyzer::detectLanguage(const std::string& content) {
    auto tokens = tokenize(content);
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = languageModel.forward(inputs).toTensor();
    
    auto predicted = std::get<0>(torch::max(output, 0));
    return std::to_string(predicted.item<int>());
}

std::vector<std::string> ContentAnalyzer::classifyTopics(const std::string& content) {
    auto tokens = tokenize(content);
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = topicModel.forward(inputs).toTensor();
    
    std::vector<std::string> topics;
    auto indices = std::get<0>(torch::nonzero(output > 0.5));
    
    for (int i = 0; i < indices.size(0); i++) {
        topics.push_back(std::to_string(indices[i].item<int>()));
    }
    
    return topics;
}

bool ContentAnalyzer::detectSpam(const std::string& content) {
    auto tokens = tokenize(content);
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = spamModel.forward(inputs).toTensor();
    
    return output.item<double>() > 0.5;
}

std::vector<std::string> ContentAnalyzer::extractNamedEntities(const std::string& content) {
    auto tokens = tokenize(content);
    torch::Tensor input = torch::from_blob(tokens.data(),
        {(long)tokens.size()}, torch::kFloat32);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input);
    auto output = entityModel.forward(inputs).toTensor();
    
    std::vector<std::string> entities;
    auto indices = std::get<0>(torch::nonzero(output > 0.5));
    
    for (int i = 0; i < indices.size(0); i++) {
        entities.push_back(std::to_string(indices[i].item<int>()));
    }
    
    return entities;
} 