//
//  trayraceCompose.hpp
//  trayrace
//
//  Created by Josh McNamee on 20/08/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceCompose_hpp
#define trayrace_trayraceCompose_hpp


#include <Mush Core/imageProcess.hpp>

class trayraceCompose : public mush::imageProcess {
public:
    trayraceCompose() : mush::imageProcess() {
        
    }
    
    ~trayraceCompose() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
        
//        copyImage = context->getKernel("copyImage");
        //        fromCache = context->getKernel("fromCache");
        historyBuffer = context->getKernel("historyBuffer");
        spatiotemporalUpsample = context->getKernel("spatiotemporalUpsample");
        fromRay = context->getKernel("fromRay");
        spatialClear = context->getKernel("spatialClear");
        
        if (buffers.size() < 2) {
            putLog("compose kernel: not enough buffers");
        }
        
        rays = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[0]);
        if (rays.get() == nullptr) {
            putLog("compose kernel: bad ray buffer");
            return;
        }
        
        maps = std::dynamic_pointer_cast<mush::ringBuffer>(buffers.begin()[1]);
        if (maps.get() == nullptr) {
            putLog("compose kernel: bad map buffer");
            return;
        }
        
        motion = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[2]);
        if (motion.get() == nullptr) {
            putLog("compose kernel: bad mption buffer");
            return;
        }
        
        upsample = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[3]);
        if (upsample.get() == nullptr) {
            putLog("compose kernel: bad upsample buffer");
            return;
        }
        
        rays->getParams(_width, _height, _size);
        
        historyFrame = context->floatImage(_width, _height);
        
        addItem(context->floatImage(_width, _height));
        
        fromRay->setArg(0, _getImageMem(0));
        
        spatiotemporalUpsample->setArg(0, _getImageMem(0));
        spatiotemporalUpsample->setArg(1, *historyFrame);
        
        historyBuffer->setArg(0, *historyFrame);
        historyBuffer->setArg(1, _getImageMem(0));
        historyBuffer->setArg(2, _getImageMem(0));
        
        queue = context->getQueue();
        cl::Event event;
        spatialClear->setArg(0, *historyFrame);
        queue->enqueueNDRangeKernel(*spatialClear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
//        fromCache->setArg(0, *_getImageMem(0)));
//        fromCache->setArg(1, *previousFrame);
        
//        copyImage->setArg(0, *_getImageMem(0)));
//        copyImage->setArg(1, *previousFrame);
        
        
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        
        auto map = maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
        auto ray = rays->outLock();
        if (ray == nullptr) {
            release();
            return;
        }
        
        auto m = motion->outLock();
        if (m == nullptr) {
            release();
            return;
        }
        
        auto up = upsample->outLock();
        if (up == nullptr) {
            release();
            return;
        }
        
        fromRay->setArg(1, ray.get_image());
        fromRay->setArg(2, map.get_buffer());
        
        spatiotemporalUpsample->setArg(2, up.get_image());
        spatiotemporalUpsample->setArg(3, map.get_buffer());
        spatiotemporalUpsample->setArg(4, m.get_image());
        
        queue->enqueueNDRangeKernel(*spatiotemporalUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        upsample->outUnlock();
        motion->outUnlock();
        
        //queue->enqueueNDRangeKernel(*fromRay, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        //event.wait();
        
        rays->outUnlock();
        maps->outUnlock();
        
        queue->enqueueNDRangeKernel(*historyBuffer, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * historyBuffer = nullptr;
    cl::Kernel * spatiotemporalUpsample = nullptr;
    cl::Kernel * fromRay = nullptr;
    cl::Kernel * spatialClear = nullptr;
    
    cl::Image2D * historyFrame = nullptr;
    
    mush::registerContainer<mush::imageBuffer> rays;
	mush::registerContainer<mush::ringBuffer> maps;
	mush::registerContainer<mush::imageBuffer> motion;
	mush::registerContainer<mush::imageBuffer> upsample;
    
    int count = 0;
    
};

#endif
