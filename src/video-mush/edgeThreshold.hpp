//
//  laplaceEncoderProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_laplaceEncoderProcess_hpp
#define video_mush_laplaceEncoderProcess_hpp

#include <Video Mush/imageProcess.hpp>

class edgeThreshold : public mush::imageProcess {
public:
    edgeThreshold() : mush::imageProcess() {
        
    }
    
    ~edgeThreshold() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        threshold = context->getKernel("edge_threshold");
        samples = context->getKernel("edge_samples");
        clear = context->getKernel("edge_clear");
        
        if (buffers.size() < 2) {
            putLog("laplace kernel: not enough buffers");
        }
        
        imageBuffe = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[0]);
        if (imageBuffe == nullptr) {
            putLog("edge threshold kernel: bad image buffer");
            return;
        }
        
        imageBuffe->getParams(_width, _height, _size);
        
        laplaceBuffer = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]);
        if (laplaceBuffer == nullptr) {
            putLog("edge threshold kernel: bad edge buffer");
            return;
        }
        
        addItem((unsigned char *)context->floatImage(_width, _height));
        
        
        clear->setArg(0, *_getImageMem(0));
        threshold->setArg(2, *_getImageMem(0));
        samples->setArg(1, *_getImageMem(0));
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        cl::Image2D const * image = imageBuffe->imageOutLock();
        if (image == nullptr) {
            release();
            return;
        }
        
        cl::Image2D const * laplace = laplaceBuffer->imageOutLock();
        if (laplace == nullptr) {
            release();
            return;
        }
        
        threshold->setArg(0, *image);
        threshold->setArg(1, *laplace);
        queue->enqueueNDRangeKernel(*threshold, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        samples->setArg(0, *image);
        queue->enqueueNDRangeKernel(*samples, cl::NullRange, cl::NDRange((_width - (_width % 32))/32, (_height - (_height %32))/32), cl::NullRange, NULL, &event);
        event.wait();
        
        laplaceBuffer->outUnlock();
        imageBuffe->outUnlock();
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * threshold = nullptr;
    cl::Kernel * samples = nullptr;
    cl::Kernel * clear = nullptr;
    
    std::shared_ptr<mush::imageBuffer> laplaceBuffer;
    std::shared_ptr<mush::imageBuffer> imageBuffe;
};

#endif

