//
//  motionProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_gmotionProcess_hpp
#define video_mush_gmotionProcess_hpp

#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/tagInGui.hpp>

class getDiscontinuities : public mush::integerMapProcess {
public:
    getDiscontinuities() : mush::integerMapProcess() {
        
    }
    
    ~getDiscontinuities() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
        
        discontinuities = context->getKernel("discontinuities");
        copyImage = context->getKernel("copyImage");
    
        //depthDiff = context->getKernel("depthDiff");
        
        if (buffers.size() < 2) {
            putLog("discontinuities kernel: not enough buffers");
        }
        
        motionVectors = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[0]);
        if (motionVectors == nullptr) {
            putLog("discontinuities kernel: bad motion buffer");
            return;
        }
        
        depth = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]);
        if (depth == nullptr) {
            putLog("discontinuities kernel: bad depth buffer");
            return;
        }
        
        depth->getParams(_width, _height, _size);
        
        previousDepth = context->floatImage(_width, _height);
        
        addItem(context->buffer(_width*_height*sizeof(cl_uchar)));
        
        
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        cl::Event event;
        
        auto mot = motionVectors->outLock();
        if (mot == nullptr) {
            release();
            return;
        }
        
        auto inDepth = depth->outLock();
        if (inDepth == nullptr) {
            release();
            return;
        }

		discontinuities->setArg(0, _getMem(0).get_buffer());
        discontinuities->setArg(1, mot.get_image());
        discontinuities->setArg(2, inDepth.get_image());
        discontinuities->setArg(3, *temp_image);
        queue->enqueueNDRangeKernel(*discontinuities, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
		inUnlock();
        //inUnlock(mot, inDepth);
        
        motionVectors->outUnlock();
        
        depth->outUnlock();
        
    }
    /*
    virtual void inUnlock(mush::buffer& mot, mush::buffer& depth) {
        if (getTagInGuiMember() && tagGui != nullptr) {
            
            depthDiff->setArg(0, *temp_image);
            depthDiff->setArg(1, mot.get_image());
            depthDiff->setArg(2, depth.get_image());
            cl::Event event;
            queue->enqueueNDRangeKernel(*depthDiff, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
			mush::buffer temp{ *temp_image };
            tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), temp);
        }
        ringBuffer::inUnlock();
    }
    */
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * discontinuities = nullptr;
    cl::Kernel * copyImage = nullptr;
    
    cl::Kernel * depthDiff = nullptr;
    
    cl::Image2D * previousDepth = nullptr;
    
    mush::registerContainer<mush::imageBuffer> motionVectors;
	mush::registerContainer<mush::imageBuffer> depth;
};
#endif
