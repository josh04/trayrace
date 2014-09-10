//
//  trayraceUpsample.hpp
//  trayrace
//
//  Created by Josh McNamee on 07/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceUpsample_hpp
#define trayrace_trayraceUpsample_hpp

#include <Video Mush/imageProcess.hpp>

class trayraceUpsample : public mush::imageProcess {
public:
    trayraceUpsample() : mush::imageProcess() {
        
    }
    
    ~trayraceUpsample() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        spatialClear = context->getKernel("spatialClear");
        scaleImage = context->getKernel("scaleImage");
        spatialUpsample = context->getKernel("spatialUpsample");
        
        if (buffers.size() < 4) {
            putLog("upsample kernel: not enough buffers");
        }
        
        maps = std::dynamic_pointer_cast<mush::ringBuffer>(buffers[0]);
        if (maps == nullptr) {
            putLog("upsample kernel: bad map buffer");
            return;
        }
        
        geometry = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]);
        if (geometry == nullptr) {
            putLog("geometry kernel: bad geometry buffer");
            return;
        }
        
        depth = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]);
        if (depth == nullptr) {
            putLog("depth kernel: bad mption buffer");
            return;
        }
        
        redraw = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]);
        if (redraw == nullptr) {
            putLog("redraw kernel: bad mption buffer");
            return;
        }
        
        geometry->getParams(_width, _height, _size);
        
        
        lowRes = context->floatImage(_width/4, _height/4);
        addItem(context->floatImage(_width, _height));
        
        spatialClear->setArg(0, *(cl::Image2D *)_getMem(0));
        
        scaleImage->setArg(0, *lowRes);
        
        spatialUpsample->setArg(0, *(cl::Image2D *)_getMem(0));
        spatialUpsample->setArg(1, *(cl::Image2D *)_getMem(0));
        spatialUpsample->setArg(2, *lowRes);
        
        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        queue->enqueueNDRangeKernel(*spatialClear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        cl::Buffer * map = (cl::Buffer *)maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
        cl::Image2D * geom = (cl::Image2D *)geometry->outLock();
        if (geom == nullptr) {
            release();
            return;
        }
        
        cl::Image2D * d = (cl::Image2D *)depth->outLock();
        if (d == nullptr) {
            release();
            return;
        }
        
        cl::Image2D * re = (cl::Image2D *)redraw->outLock();
        if (re == nullptr) {
            release();
            return;
        }
        
        scaleImage->setArg(1, *re);
        scaleImage->setArg(2, *map);
        queue->enqueueNDRangeKernel(*scaleImage, cl::NullRange, cl::NDRange(_width/4, _height/4), cl::NullRange, NULL, &event);
        event.wait();
        
        spatialUpsample->setArg(3, *geom);
        spatialUpsample->setArg(4, *d);
        spatialUpsample->setArg(5, *map);
        
        // BLOCK 'O' FIVE /*FOUR*/

        spatialUpsample->setArg(6, (char)0);
        spatialUpsample->setArg(7, (char)0);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
		
		spatialUpsample->setArg(6, (char)-1);
		spatialUpsample->setArg(7, (char)0);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
		spatialUpsample->setArg(6, (char)1);
		spatialUpsample->setArg(7, (char)0);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
		spatialUpsample->setArg(6, (char)0);
		spatialUpsample->setArg(7, (char)-1);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
		spatialUpsample->setArg(6, (char)0);
		spatialUpsample->setArg(7, (char)1);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        // END
        
        redraw->outUnlock();
        depth->outUnlock();
        geometry->outUnlock();
        maps->outUnlock();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * spatialClear = nullptr;
    cl::Kernel * scaleImage = nullptr;
    cl::Kernel * spatialUpsample = nullptr;
    
    std::shared_ptr<mush::ringBuffer> maps = nullptr;
    std::shared_ptr<mush::imageBuffer> geometry = nullptr;
    std::shared_ptr<mush::imageBuffer> depth = nullptr;
    std::shared_ptr<mush::imageBuffer> redraw = nullptr;
    
    cl::Image2D * lowRes = nullptr;
    
    int count = 0;
    
};

#endif
