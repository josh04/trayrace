//
//  laplaceEncoderProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_laplaceEncoderProcess_hpp
#define video_mush_laplaceEncoderProcess_hpp

#include <Mush Core/imageProcess.hpp>

class edgeThreshold : public mush::imageProcess {
public:
    edgeThreshold() : mush::imageProcess() {
        
    }
    
    ~edgeThreshold() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
        
        threshold = context->getKernel("edge_threshold");
        samples = context->getKernel("edge_samples");
        clear = context->getKernel("edge_clear");
        
        if (buffers.size() < 2) {
            putLog("laplace kernel: not enough buffers");
        }
        
        imageBuffe = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[0]);
        if (imageBuffe == nullptr) {
            putLog("edge threshold kernel: bad image buffer");
            return;
        }
        
        imageBuffe->getParams(_width, _height, _size);
        
        laplaceBuffer = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]);
        if (laplaceBuffer == nullptr) {
            putLog("edge threshold kernel: bad edge buffer");
            return;
        }
        
        addItem(context->floatImage(_width, _height));
        
        
        clear->setArg(0, _getImageMem(0));
        samples->setArg(1, _getImageMem(0));
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        auto image = imageBuffe->outLock();
        if (image == nullptr) {
            release();
            return;
        }
        
		auto laplace = laplaceBuffer->outLock();
        if (laplace == nullptr) {
            release();
            return;
        }
        
        threshold->setArg(0, image.get_image());
        threshold->setArg(1, laplace.get_image());
		threshold->setArg(2, _getImageMem(0));
		threshold->setArg(3, 0.1f);
        queue->enqueueNDRangeKernel(*threshold, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        samples->setArg(0, image.get_image());
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
    
    mush::registerContainer<mush::imageBuffer> laplaceBuffer;
	mush::registerContainer<mush::imageBuffer> imageBuffe;
};

#endif

