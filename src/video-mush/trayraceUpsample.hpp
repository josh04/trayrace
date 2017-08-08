//
//  trayraceUpsample.hpp
//  trayrace
//
//  Created by Josh McNamee on 07/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceUpsample_hpp
#define trayrace_trayraceUpsample_hpp

#include <Mush Core/imageProcess.hpp>

class trayraceUpsample : public mush::imageProcess {
public:
    trayraceUpsample() : mush::imageProcess() {
        
    }
    
    ~trayraceUpsample() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
        
        spatialClear = context->getKernel("spatialClear");
        scaleImage = context->getKernel("scaleImage");
        spatialUpsample = context->getKernel("spatialUpsample");
        
        if (buffers.size() < 4) {
            putLog("upsample kernel: not enough buffers");
        }
        
        maps = std::dynamic_pointer_cast<mush::ringBuffer>(buffers.begin()[0]);
        if (maps.get() == nullptr) {
            putLog("upsample kernel: bad map buffer");
            return;
        }
        
        geometry = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]);
        if (geometry.get() == nullptr) {
            putLog("geometry kernel: bad geometry buffer");
            return;
        }
        
        depth = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[2]);
        if (depth.get() == nullptr) {
            putLog("depth kernel: bad mption buffer");
            return;
        }
        
        redraw = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[3]);
        if (redraw.get() == nullptr) {
            putLog("redraw kernel: bad mption buffer");
            return;
        }
        
        geometry->getParams(_width, _height, _size);
        
        
        lowRes = context->floatImage(_width/4, _height/4);
        addItem(context->floatImage(_width, _height));
        
        spatialClear->setArg(0, _getImageMem(0));
        
        
        spatialUpsample->setArg(0, _getImageMem(0));
        spatialUpsample->setArg(1, _getImageMem(0));
        spatialUpsample->setArg(2, *lowRes);
		spatialUpsample->setArg(10, 4);

		setParams(0, 0.04, 0.01, 10.0);

        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        queue->enqueueNDRangeKernel(*spatialClear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
		auto map = maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
		auto geom = geometry->outLock();
        if (geom == nullptr) {
            release();
            return;
        }
        
		auto d = depth->outLock();
        if (d == nullptr) {
            release();
            return;
        }
        
        auto re = redraw->outLock();
        if (re == nullptr) {
            release();
            return;
        }
        
        count = (count+1) % 16;
        spatialUpsample->setArg(8, count);

		scaleImage->setArg(0, *lowRes);
        scaleImage->setArg(1, re.get_image());
        scaleImage->setArg(2, map.get_buffer());
		scaleImage->setArg(3, 4);
        queue->enqueueNDRangeKernel(*scaleImage, cl::NullRange, cl::NDRange(_width/4, _height/4), cl::NullRange, NULL, &event);
        event.wait();
        
        spatialUpsample->setArg(3, geom.get_image());
        spatialUpsample->setArg(4, d.get_image());
        spatialUpsample->setArg(5, map.get_buffer());
        spatialUpsample->setArg(9, re.get_image());
        
        // BLOCK 'O' FIVE /*FOUR*/
		
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
        
        
        spatialUpsample->setArg(6, (char)0);
        spatialUpsample->setArg(7, (char)0);
        queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        // END
        
        redraw->outUnlock();
        depth->outUnlock();
        geometry->outUnlock();
        maps->outUnlock();
        
        inUnlock();
    }


	void setParams(int count, float geomWeight, float depthWeight, float kWeight) {

		spatialUpsample->setArg(8, count - 1);
		spatialUpsample->setArg(11, geomWeight);
		spatialUpsample->setArg(12, depthWeight);
		spatialUpsample->setArg(13, kWeight);
		this->count = count - 1;

	}

private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * spatialClear = nullptr;
    cl::Kernel * scaleImage = nullptr;
    cl::Kernel * spatialUpsample = nullptr;
    
    mush::registerContainer<mush::ringBuffer> maps = nullptr;
	mush::registerContainer<mush::imageBuffer> geometry = nullptr;
	mush::registerContainer<mush::imageBuffer> depth = nullptr;
	mush::registerContainer<mush::imageBuffer> redraw = nullptr;
    
    cl::Image2D * lowRes = nullptr;
    
    int count = 0;
    
};

#endif
