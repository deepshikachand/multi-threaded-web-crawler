#include "image_analyzer.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

ImageAnalyzer::ImageAnalyzer() {
    // Initialize OCR
    ocr = std::make_unique<tesseract::TessBaseAPI>();
    if (ocr->Init(nullptr, "eng")) {
        throw std::runtime_error("Failed to initialize Tesseract OCR");
    }

    // Load ML models
    try {
        loadModels("models");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize ML models: " + std::string(e.what()));
    }
}

ImageAnalyzer::~ImageAnalyzer() {
    if (ocr) {
        ocr->End();
    }
}

ImageAnalyzer::ImageFeatures ImageAnalyzer::analyzeImage(const std::string& imagePath) {
    ImageFeatures features;
    
    // Load and preprocess image
    cv::Mat image = preprocessImage(imagePath);
    
    // Run analysis
    auto labels = classifyImage(image);
    auto objects = detectObjects(image);
    bool nsfw = detectNSFW(image);
    
    // Extract text using OCR
    std::string text = extractText(imagePath);
    
    // Generate description
    std::string description = generateCaption(image, labels);
    
    // Fill features
    features.labels = labels;
    features.objects = std::vector<std::string>(objects.size());
    features.sizes = std::vector<std::pair<int, int>>(objects.size());
    
    for (size_t i = 0; i < objects.size(); ++i) {
        features.objects[i] = objects[i].first;
        features.confidence.push_back(objects[i].second);
    }
    
    features.ocrText = text;
    features.description = description;
    features.isNSFW = nsfw;
    
    return features;
}

ImageAnalyzer::ImageFeatures ImageAnalyzer::analyzeImageData(const std::vector<uint8_t>& imageData) {
    ImageFeatures features;
    
    // Preprocess image data
    cv::Mat image = preprocessImageData(imageData);
    
    // Run analysis
    auto labels = classifyImage(image);
    auto objects = detectObjects(image);
    bool nsfw = detectNSFW(image);
    
    // Extract text using OCR
    std::string text = extractTextFromData(imageData);
    
    // Generate description
    std::string description = generateCaption(image, labels);
    
    // Fill features
    features.labels = labels;
    features.objects = std::vector<std::string>(objects.size());
    features.sizes = std::vector<std::pair<int, int>>(objects.size());
    
    for (size_t i = 0; i < objects.size(); ++i) {
        features.objects[i] = objects[i].first;
        features.confidence.push_back(objects[i].second);
    }
    
    features.ocrText = text;
    features.description = description;
    features.isNSFW = nsfw;
    
    return features;
}

std::string ImageAnalyzer::generateDescription(const ImageFeatures& features) {
    std::stringstream ss;
    
    // Add object detection results
    ss << "Detected objects: ";
    for (size_t i = 0; i < features.objects.size(); ++i) {
        ss << features.objects[i] << " (" << features.confidence[i] << "), ";
    }
    
    // Add classification labels
    ss << "\nImage categories: ";
    for (const auto& label : features.labels) {
        ss << label << ", ";
    }
    
    // Add OCR text if available
    if (!features.ocrText.empty()) {
        ss << "\nExtracted text: " << features.ocrText;
    }
    
    return ss.str();
}

bool ImageAnalyzer::isNSFW(const std::string& imagePath) {
    cv::Mat image = preprocessImage(imagePath);
    return detectNSFW(image);
}

std::vector<std::string> ImageAnalyzer::detectObjects(const std::string& imagePath) {
    cv::Mat image = preprocessImage(imagePath);
    auto objects = detectObjects(image);
    
    std::vector<std::string> objectNames;
    for (const auto& obj : objects) {
        objectNames.push_back(obj.first);
    }
    
    return objectNames;
}

std::string ImageAnalyzer::extractText(const std::string& imagePath) {
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        throw std::runtime_error("Failed to load image: " + imagePath);
    }
    
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // Apply thresholding
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    
    // Set image for OCR
    ocr->SetImage(binary.data, binary.cols, binary.rows, 1, binary.step);
    
    // Get text
    char* outText = ocr->GetUTF8Text();
    std::string text(outText);
    delete[] outText;
    
    return text;
}

std::string ImageAnalyzer::extractTextFromData(const std::vector<uint8_t>& imageData) {
    cv::Mat image = cv::imdecode(imageData, cv::IMREAD_COLOR);
    if (image.empty()) {
        throw std::runtime_error("Failed to decode image data");
    }
    
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    
    // Apply thresholding
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    
    // Set image for OCR
    ocr->SetImage(binary.data, binary.cols, binary.rows, 1, binary.step);
    
    // Get text
    char* outText = ocr->GetUTF8Text();
    std::string text(outText);
    delete[] outText;
    
    return text;
}

void ImageAnalyzer::loadModels(const std::string& modelDir) {
    try {
        classificationModel = torch::jit::load(modelDir + "/image_classifier.pt");
        objectDetectionModel = torch::jit::load(modelDir + "/object_detector.pt");
        nsfwModel = torch::jit::load(modelDir + "/nsfw_detector.pt");
        captionModel = torch::jit::load(modelDir + "/image_captioner.pt");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load models: " + std::string(e.what()));
    }
}

void ImageAnalyzer::saveModels(const std::string& modelDir) {
    try {
        classificationModel.save(modelDir + "/image_classifier.pt");
        objectDetectionModel.save(modelDir + "/object_detector.pt");
        nsfwModel.save(modelDir + "/nsfw_detector.pt");
        captionModel.save(modelDir + "/image_captioner.pt");
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to save models: " + std::string(e.what()));
    }
}

cv::Mat ImageAnalyzer::preprocessImage(const std::string& imagePath) {
    cv::Mat image = cv::imread(imagePath);
    if (image.empty()) {
        throw std::runtime_error("Failed to load image: " + imagePath);
    }
    
    // Resize image
    image = resizeImage(image, 224); // Standard size for many vision models
    
    // Normalize image
    image = normalizeImage(image);
    
    return image;
}

cv::Mat ImageAnalyzer::preprocessImageData(const std::vector<uint8_t>& imageData) {
    cv::Mat image = cv::imdecode(imageData, cv::IMREAD_COLOR);
    if (image.empty()) {
        throw std::runtime_error("Failed to decode image data");
    }
    
    // Resize image
    image = resizeImage(image, 224);
    
    // Normalize image
    image = normalizeImage(image);
    
    return image;
}

torch::Tensor ImageAnalyzer::imageToTensor(const cv::Mat& image) {
    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
    
    // Convert to float and normalize
    cv::Mat float_img;
    rgb.convertTo(float_img, CV_32F, 1.0/255.0);
    
    // Create tensor
    auto tensor = torch::from_blob(float_img.data, 
        {1, float_img.rows, float_img.cols, 3}, torch::kFloat32);
    
    // Permute to NCHW format
    tensor = tensor.permute({0, 3, 1, 2});
    
    return tensor;
}

std::vector<std::string> ImageAnalyzer::classifyImage(const cv::Mat& image) {
    auto tensor = imageToTensor(image);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
    auto output = classificationModel.forward(inputs).toTensor();
    
    auto probs = softmax(output[0]);
    auto labels = loadLabels("models/imagenet_labels.txt");
    
    std::vector<std::string> results;
    for (size_t i = 0; i < probs.size(); ++i) {
        if (probs[i] > 0.1) { // Confidence threshold
            results.push_back(labels[i]);
        }
    }
    
    return results;
}

std::vector<std::pair<std::string, float>> ImageAnalyzer::detectObjects(const cv::Mat& image) {
    auto tensor = imageToTensor(image);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
    auto output = objectDetectionModel.forward(inputs).toTuple();
    
    auto boxes = output->elements()[0].toTensor();
    auto scores = output->elements()[1].toTensor();
    auto labels = output->elements()[2].toTensor();
    
    auto labelNames = loadLabels("models/coco_labels.txt");
    
    std::vector<std::pair<std::string, float>> results;
    for (int i = 0; i < scores.size(0); ++i) {
        if (scores[i].item<float>() > 0.5) { // Confidence threshold
            int labelIdx = labels[i].item<int>();
            results.emplace_back(labelNames[labelIdx], scores[i].item<float>());
        }
    }
    
    return results;
}

bool ImageAnalyzer::detectNSFW(const cv::Mat& image) {
    auto tensor = imageToTensor(image);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
    auto output = nsfwModel.forward(inputs).toTensor();
    
    return output[0][1].item<float>() > 0.5; // NSFW probability threshold
}

std::string ImageAnalyzer::generateCaption(const cv::Mat& image, const std::vector<std::string>& objects) {
    auto tensor = imageToTensor(image);
    
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);
    auto output = captionModel.forward(inputs).toTensor();
    
    // Convert output to text (simplified)
    std::stringstream ss;
    ss << "A ";
    for (const auto& obj : objects) {
        ss << obj << " ";
    }
    ss << "in the image.";
    
    return ss.str();
}

std::vector<float> ImageAnalyzer::softmax(const torch::Tensor& tensor) {
    auto exp = torch::exp(tensor);
    auto sum = exp.sum();
    return std::vector<float>(exp.data_ptr<float>(), 
        exp.data_ptr<float>() + exp.numel());
}

std::vector<std::string> ImageAnalyzer::loadLabels(const std::string& labelFile) {
    std::vector<std::string> labels;
    std::ifstream file(labelFile);
    std::string line;
    
    while (std::getline(file, line)) {
        labels.push_back(line);
    }
    
    return labels;
}

cv::Mat ImageAnalyzer::resizeImage(const cv::Mat& image, int targetSize) {
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(targetSize, targetSize));
    return resized;
}

cv::Mat ImageAnalyzer::normalizeImage(const cv::Mat& image) {
    cv::Mat normalized;
    image.convertTo(normalized, CV_32F, 1.0/255.0);
    
    // Normalize using ImageNet mean and std
    float mean[3] = {0.485, 0.456, 0.406};
    float std[3] = {0.229, 0.224, 0.225};
    
    std::vector<cv::Mat> channels;
    cv::split(normalized, channels);
    
    for (int i = 0; i < 3; ++i) {
        channels[i] = (channels[i] - mean[i]) / std[i];
    }
    
    cv::merge(channels, normalized);
    return normalized;
} 