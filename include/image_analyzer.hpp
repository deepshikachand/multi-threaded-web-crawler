#pragma once

#include <string>
#include <vector>
#include <memory>
#include <torch/torch.h>
#include <torch/script.h>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

class ImageAnalyzer {
public:
    struct ImageFeatures {
        std::vector<std::string> labels;        // Image classification labels
        std::vector<float> confidence;          // Confidence scores for labels
        std::string ocrText;                   // Extracted text from image
        std::string description;                // Generated image description
        std::vector<std::string> objects;       // Detected objects
        std::vector<std::pair<int, int>> sizes; // Object bounding boxes
        bool isNSFW;                           // NSFW content flag
    };

    ImageAnalyzer();
    ~ImageAnalyzer();

    // Image analysis
    ImageFeatures analyzeImage(const std::string& imagePath);
    ImageFeatures analyzeImageData(const std::vector<uint8_t>& imageData);
    std::string generateDescription(const ImageFeatures& features);
    bool isNSFW(const std::string& imagePath);
    std::vector<std::string> detectObjects(const std::string& imagePath);

    // OCR functionality
    std::string extractText(const std::string& imagePath);
    std::string extractTextFromData(const std::vector<uint8_t>& imageData);

    // Model management
    void loadModels(const std::string& modelDir);
    void saveModels(const std::string& modelDir);

private:
    // ML models
    torch::jit::Module classificationModel;
    torch::jit::Module objectDetectionModel;
    torch::jit::Module nsfwModel;
    torch::jit::Module captionModel;

    // OCR engine
    std::unique_ptr<tesseract::TessBaseAPI> ocr;

    // Image preprocessing
    cv::Mat preprocessImage(const std::string& imagePath);
    cv::Mat preprocessImageData(const std::vector<uint8_t>& imageData);
    torch::Tensor imageToTensor(const cv::Mat& image);
    
    // Model inference
    std::vector<std::string> classifyImage(const cv::Mat& image);
    std::vector<std::pair<std::string, float>> detectObjects(const cv::Mat& image);
    bool detectNSFW(const cv::Mat& image);
    std::string generateCaption(const cv::Mat& image, const std::vector<std::string>& objects);

    // Helper functions
    std::vector<float> softmax(const torch::Tensor& tensor);
    std::vector<std::string> loadLabels(const std::string& labelFile);
    cv::Mat resizeImage(const cv::Mat& image, int targetSize);
    cv::Mat normalizeImage(const cv::Mat& image);
}; 