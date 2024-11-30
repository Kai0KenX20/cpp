#ifndef DETECTOR_H
#define DETECTOR_H

#include <opencv2/opencv.hpp>
#include "NvInfer.h"
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include <cuda_fp16.h>
#include <memory>

struct DetResult
{
    cv::Rect bbox;
    float conf;
    int label;

    DetResult(cv::Rect b, float c, int l) : bbox(b), conf(c), label(l) {}
};

class Detector
{
public:
    Detector();
    ~Detector();
    void initialize(const std::string& modelFile);
    void processFrame(const cv::cuda::GpuMat& frame);
    void inferenceThread();
    void releaseDetections();
    bool getLatestDetections(std::vector<cv::Rect>& boxes, std::vector<int>& classes);

    int detectionVersion;
    std::mutex detectionMutex;
    std::condition_variable detectionCV;
    std::vector<cv::Rect> detectedBoxes;
    std::vector<int> detectedClasses;

private:
    std::unique_ptr<nvinfer1::IRuntime> runtime;
    std::unique_ptr<nvinfer1::ICudaEngine> engine;
    std::unique_ptr<nvinfer1::IExecutionContext> context;
    nvinfer1::Dims inputDims;
    cudaStream_t stream;

    std::mutex inferenceMutex;
    std::condition_variable inferenceCV;
    std::atomic<bool> shouldExit;
    cv::cuda::GpuMat currentFrame;
    bool frameReady;

    void loadEngine(const std::string& engineFile);
    void preProcess(const cv::cuda::GpuMat& frame);
    void postProcess(const float* output, int outputSize);

    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::unordered_map<std::string, size_t> inputSizes;
    std::unordered_map<std::string, size_t> outputSizes;
    std::unordered_map<std::string, void*> inputBindings;
    std::unordered_map<std::string, void*> outputBindings;
    std::unordered_map<std::string, std::vector<int64_t>> outputShapes;

    size_t getSizeByDim(const nvinfer1::Dims& dims);
    size_t getElementSize(nvinfer1::DataType dtype);
    void getInputNames();
    void getOutputNames();
    void getBindings();

    std::vector<float> inputBuffer;
    std::string inputName;
    void* inputBufferDevice;
    std::unordered_map<std::string, std::vector<float>> outputDataBuffers;
    std::unordered_map<std::string, std::vector<__half>> outputDataBuffersHalf;
    std::unordered_map<std::string, nvinfer1::DataType> outputTypes;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> classes;
    float scale;
    std::vector<cv::Mat> channels;
};

#endif // DETECTOR_H