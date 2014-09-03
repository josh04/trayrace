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
        
        copyImage = context->getKernel("copyImage");
        fromCache = context->getKernel("fromCache");
        fromRay = context->getKernel("fromRay");
        
        if (buffers.size() < 2) {
            putLog("compose kernel: not enough buffers");
        }
        
        rays = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[0]);
        if (rays == nullptr) {
            putLog("compose kernel: bad ray buffer");
            return;
        }
        
        maps = std::dynamic_pointer_cast<trayraceRedraw>(buffers[1]);
        if (maps == nullptr) {
            putLog("compose kernel: bad map buffer");
            return;
        }
        
        motion = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]);
        if (motion == nullptr) {
            putLog("compose kernel: bad mption buffer");
            return;
        }
        
        rays->getParams(_width, _height, _size);
        
        previousFrame = context->floatImage(_width, _height);
        
        addItem(context->floatImage(_width, _height));
        
        fromRay->setArg(0, *(cl::Image2D *)_getMem(0));
        
        fromCache->setArg(0, *(cl::Image2D *)_getMem(0));
        fromCache->setArg(1, *previousFrame);
        
        copyImage->setArg(0, *(cl::Image2D *)_getMem(0));
        copyImage->setArg(1, *previousFrame);
        
        
        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        
        cl::Buffer * map = (cl::Buffer *)maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
        cl::Image2D * ray = (cl::Image2D *)rays->outLock();
        if (ray == nullptr) {
            release();
            return;
        }
        
        cl::Image2D * m = (cl::Image2D *)motion->outLock();
        if (m == nullptr) {
            release();
            return;
        }
        
        fromRay->setArg(1, *ray);
        fromRay->setArg(2, *map);
        fromCache->setArg(2, *m);
        fromCache->setArg(3, *map);
        
        queue->enqueueNDRangeKernel(*fromCache, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        motion->outUnlock();
        
        queue->enqueueNDRangeKernel(*fromRay, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        rays->outUnlock();
        maps->outUnlock();
        
        queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * fromCache = nullptr;
    cl::Kernel * fromRay = nullptr;
    cl::Kernel * copyImage = nullptr;
    
    cl::Image2D * previousFrame = nullptr;
    
    std::shared_ptr<mush::imageBuffer> rays;
    std::shared_ptr<trayraceRedraw> maps;
    std::shared_ptr<mush::imageBuffer> motion;
    
    int count = 0;
};

#endif
