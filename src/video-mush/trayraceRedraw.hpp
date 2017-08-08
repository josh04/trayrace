//
//  trayraceRedraw.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/08/2014.
//
//

#ifndef video_mush_trayraceRedraw_hpp
#define video_mush_trayraceRedraw_hpp

#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class trayraceRedraw : public mush::integerMapProcess {
public:
    trayraceRedraw() : mush::integerMapProcess(), mapBuffer() {
        
    }
    
    ~trayraceRedraw() {
        
    }
    
    void release() {
        mapBuffer->kill();
        integerMapProcess::release();
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
		mapBuffer = std::make_shared<mush::ringBuffer>();
        redraw = context->getKernel("redraw");
        
        if (buffers.size() < 1) {
            putLog("redraw kernel: not enough buffers");
        }
        
        discontinuities = std::dynamic_pointer_cast<mush::integerMapBuffer>(buffers.begin()[0]);
        if (discontinuities == nullptr) {
            putLog("redraw kernel: bad discont buffer");
            return;
        }
        
        discontinuities->getParams(_width, _height, _size);
        
        getMap = context->hostReadBuffer(_width*_height*sizeof(cl_uchar));
        memset(getMap, 1, _width*_height*sizeof(cl_uchar));
        mapBuffer->addItem(getMap);
        
        addItem(context->buffer(_width*_height*sizeof(cl_uchar)));
        
        
        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        auto discont = discontinuities->outLock();
        if (discont == nullptr) {
            release();
            mapBuffer->kill();
            return;
        }
        
        count = (count+1) % 16;

		redraw->setArg(0, _getMem(0).get_buffer());
        redraw->setArg(1, discont.get_buffer());
		redraw->setArg(2, _width);
        redraw->setArg(3, count);
		redraw->setArg(4, 0);
        queue->enqueueNDRangeKernel(*redraw, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        mapBuffer->inLock();
        queue->enqueueReadBuffer(_getMem(0).get_buffer(), CL_TRUE, 0, _width*_height*sizeof(cl_uchar), getMap, NULL, &event);
        event.wait();
        mapBuffer->inUnlock();
        
        discontinuities->outUnlock();
        
        inUnlock();
    }
    
	std::shared_ptr<mush::ringBuffer> getRedrawMap() {
        return mapBuffer;
    }
    
    // non-thread safe!

private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * redraw = nullptr;

    uint8_t * getMap = nullptr;
    
    mush::registerContainer<mush::integerMapBuffer> discontinuities;
    
    int count = 0;
    
    std::mutex mapMutex;
    std::shared_ptr<mush::ringBuffer> mapBuffer;
};
#endif
