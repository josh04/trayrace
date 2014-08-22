//
//  motionProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_gmotionProcess_hpp
#define video_mush_gmotionProcess_hpp

#include <Video Mush/integerMapProcess.hpp>

class getDiscontinuities : public mush::integerMapProcess {
public:
    getDiscontinuities() : mush::integerMapProcess() {
        
    }
    
    ~getDiscontinuities() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        discontinuities = context->getKernel("discontinuities");
        copyImage = context->getKernel("copyImage");
        
        if (buffers.size() < 2) {
            putLog("discontinuities kernel: not enough buffers");
        }
        
        motionVectors = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[0]);
        if (motionVectors == nullptr) {
            putLog("discontinuities kernel: bad motion buffer");
            return;
        }
        
        depth = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]);
        if (depth == nullptr) {
            putLog("discontinuities kernel: bad depth buffer");
            return;
        }
        
        depth->getParams(_width, _height, _size);
        
        previousDepth = context->floatImage(_width, _height);
        
        addItem((unsigned char *)context->buffer(_width*_height*sizeof(cl_uchar)));
        
        discontinuities->setArg(0, *((cl::Buffer *)_getMem(0)));
        
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        cl::Event event;
        
        cl::Image2D * mot = (cl::Image2D *)motionVectors->outLock();
        if (mot == nullptr) {
            release();
            return;
        }
        /*
        cl::Image2D * in = (cl::Image2D *)input->outLock();
        if (in == nullptr) {
            release();
            return;
        }*/
        
        cl::Image2D * inDepth = (cl::Image2D *)depth->outLock();
        if (inDepth == nullptr) {
            release();
            return;
        }
        /*
        copyImage->setArg(0, *previous);
        copyImage->setArg(1, *((cl::Image2D *)_getMem(0)));
        queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();*/

        discontinuities->setArg(1, *mot);
        discontinuities->setArg(2, *inDepth);
//        discontinuities->setArg(3, *in);
        queue->enqueueNDRangeKernel(*discontinuities, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        motionVectors->outUnlock();
        /*
        copyImage->setArg(0, *inDepth);
        copyImage->setArg(1, *previousDepth);
        queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        */
        depth->outUnlock();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * discontinuities = nullptr;
    cl::Kernel * copyImage = nullptr;
    
    cl::Image2D * previousDepth = nullptr;
    
    std::shared_ptr<mush::imageBuffer> motionVectors;
    std::shared_ptr<mush::imageBuffer> depth;
};
#endif
