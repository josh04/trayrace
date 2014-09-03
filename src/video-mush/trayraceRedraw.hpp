//
//  trayraceRedraw.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/08/2014.
//
//

#ifndef video_mush_trayraceRedraw_hpp
#define video_mush_trayraceRedraw_hpp

#include <Video Mush/integerMapProcess.hpp>

class trayraceRedraw : public mush::integerMapProcess {
public:
    trayraceRedraw() : mush::integerMapProcess() {
        
    }
    
    ~trayraceRedraw() {
        
    }
    
    void release() {
        mapBuffer.kill();
        integerMapProcess::release();
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        redraw = context->getKernel("redraw");
        
        if (buffers.size() < 1) {
            putLog("redraw kernel: not enough buffers");
        }
        
        discontinuities = std::dynamic_pointer_cast<mush::imageBuffer>(buffers[0]);
        if (discontinuities == nullptr) {
            putLog("redraw kernel: bad discont buffer");
            return;
        }
        
        discontinuities->getParams(_width, _height, _size);
        
        getMap = context->hostReadBuffer(_width*_height*sizeof(cl_uchar));
        memset(getMap, 1, _width*_height*sizeof(cl_uchar));
        mapBuffer.addItem(getMap);
        
        addItem(context->buffer(_width*_height*sizeof(cl_uchar)));
        
        redraw->setArg(0, *(cl::Buffer *)_getMem(0));
        redraw->setArg(2, _width);
        
        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        cl::Buffer * discont = (cl::Buffer *)discontinuities->outLock();
        if (discont == nullptr) {
            release();
            mapBuffer.kill();
            return;
        }
        
        count = (count+1) % 16;
        
        redraw->setArg(1, *discont);
        redraw->setArg(3, count);
        queue->enqueueNDRangeKernel(*redraw, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        mapBuffer.inLock();
        queue->enqueueReadBuffer(*(cl::Buffer *)_getMem(0), CL_TRUE, 0, _width*_height*sizeof(cl_uchar), getMap, NULL, &event);
        event.wait();
        mapBuffer.inUnlock();
        
        discontinuities->outUnlock();
        
        inUnlock();
    }
    
    mush::ringBuffer& getRedrawMap() {
        return std::ref(mapBuffer);
    }
    
    // non-thread safe!

private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * redraw = nullptr;

    uint8_t * getMap = nullptr;
    
    std::shared_ptr<mush::imageBuffer> discontinuities;
    
    int count = 0;
    
    std::mutex mapMutex;
    mush::ringBuffer mapBuffer;
};
#endif
