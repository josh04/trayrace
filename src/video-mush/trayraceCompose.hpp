//
//  trayraceCompose.hpp
//  trayrace
//
//  Created by Josh McNamee on 20/08/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceCompose_hpp
#define trayrace_trayraceCompose_hpp


#include <Video Mush/imageProcess.hpp>

class trayraceCompose : public mush::imageProcess {
public:
    trayraceCompose() : mush::imageProcess() {
        
    }
    
    ~trayraceCompose() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        
//        copyImage = context->getKernel("copyImage");
        //        fromCache = context->getKernel("fromCache");
        historyBuffer = context->getKernel("historyBuffer");
        spatiotemporalUpsample = context->getKernel("spatiotemporalUpsample");
        fromRay = context->getKernel("fromRay");
        spatialClear = context->getKernel("spatialClear");
        
        if (buffers.size() < 2) {
            putLog("compose kernel: not enough buffers");
        }
        
        rays = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[0]);
        if (rays == nullptr) {
            putLog("compose kernel: bad ray buffer");
            return;
        }
        
        maps = std::dynamic_pointer_cast<mush::ringBuffer>(buffers[1]);
        if (maps == nullptr) {
            putLog("compose kernel: bad map buffer");
            return;
        }
        
        motion = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]);
        if (motion == nullptr) {
            putLog("compose kernel: bad mption buffer");
            return;
        }
        
        upsample = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]);
        if (upsample == nullptr) {
            putLog("compose kernel: bad upsample buffer");
            return;
        }
        
        rays->getParams(_width, _height, _size);
        
        historyFrame = context->floatImage(_width, _height);
        
        addItem(context->floatImage(_width, _height));
        
        fromRay->setArg(0, *_getImageMem(0));
        
        spatiotemporalUpsample->setArg(0, *_getImageMem(0));
        spatiotemporalUpsample->setArg(1, *historyFrame);
        
        historyBuffer->setArg(0, *historyFrame);
        historyBuffer->setArg(1, *_getImageMem(0));
        historyBuffer->setArg(2, *_getImageMem(0));
        
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
        
        
        cl::Buffer * map = (cl::Buffer *)maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
        cl::Image2D const * ray = rays->imageOutLock();
        if (ray == nullptr) {
            release();
            return;
        }
        
        cl::Image2D const * m = motion->imageOutLock();
        if (m == nullptr) {
            release();
            return;
        }
        
        cl::Image2D const * up = upsample->imageOutLock();
        if (up == nullptr) {
            release();
            return;
        }
        
        fromRay->setArg(1, *ray);
        fromRay->setArg(2, *map);
        
        spatiotemporalUpsample->setArg(2, *up);
        spatiotemporalUpsample->setArg(3, *map);
        spatiotemporalUpsample->setArg(4, *m);
        
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
    
    std::shared_ptr<mush::imageBuffer> rays;
    std::shared_ptr<mush::ringBuffer> maps;
    std::shared_ptr<mush::imageBuffer> motion;
    std::shared_ptr<mush::imageBuffer> upsample;
    
    int count = 0;
    
};

#endif
