//
//  diffuseProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_diffuseProcess_hpp
#define video_mush_diffuseProcess_hpp

#include <Mush Core/imageProcess.hpp>

class diffuseProcess : public mush::imageProcess {
public:
    diffuseProcess() : mush::imageProcess() {
        
    }
    
    ~diffuseProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
        
        copy = context->getKernel("copyImage");
        push = context->getKernel("diffuse_push");
        pull_copy = context->getKernel("diffuse_pull_copy");
        pull_diag = context->getKernel("diffuse_pull_diag");
        pull_horiz = context->getKernel("diffuse_pull_horiz");
        
        if (buffers.size() < 1) {
            putLog("Laplace kernel: no buffers");
        }
        
        buffer = std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[0]);
        if (buffer == nullptr) {
            putLog("Diffuse kernel: bad buffers");
            return;
        }
        
        buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        
        scratch = context->floatImage(_width, _height);;
        
        first_width = (_width + 2 - (_width % 2)) / 2;
        first_height = (_height + 2 - (_height % 2)) / 2;
        first = context->floatImage(first_width, first_height);
        second_width = (first_width + 2 - (first_width % 2)) / 2;
        second_height = (first_height + 2 - (first_height % 2)) / 2;
        second = context->floatImage(second_width, second_height);
        third_width = (second_width + 2 - (second_width % 2)) / 2;
        third_height = (second_height + 2 - (second_height % 2)) / 2;
        third = context->floatImage(third_width, third_height);
        fourth_width = (third_width + 2 - (third_width % 2)) / 2;
        fourth_height = (third_height + 2 - (third_height % 2)) / 2;
        fourth = context->floatImage(fourth_width, fourth_height);
        fifth_width = (fourth_width + 2 - (fourth_width % 2)) / 2;
        fifth_height = (fourth_height + 2 - (fourth_height % 2)) / 2;
        fifth = context->floatImage(fifth_width, fifth_height);
        
        queue = context->getQueue();
    }
    
    void process() {
        auto mem = inLock();
		if (mem == nullptr) {
			release();
			return;
		}
        auto input = buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
		cl::Event event;
        copy->setArg(0, input.get_image());
        copy->setArg(1, mem.get_image());
        queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();

		//cl::Image2D image_mem = _getImageMem(0);

        //push_seq(&image_mem, first, first_width, first_height);
        push_seq(first, second, second_width, second_height);
        push_seq(second, third, third_width, third_height);
        push_seq(third, fourth, fourth_width, fourth_height);
        push_seq(fourth, fifth, fifth_width, fifth_height);
        
        
        pull_seq(fifth, fourth, fifth_width, fifth_height, fourth_width, fourth_height);
        pull_seq(fourth, third, fourth_width, fourth_height, third_width, third_height);
        pull_seq(third, second, third_width, third_height, second_width, second_height);
        pull_seq(second, first, second_width, second_height, first_width, first_height);
        //pull_seq(first, &image_mem, first_width, first_height, _width, _height);
        /*
        copy->setArg(0, *fourth);
        copy->setArg(1, *((cl::Image2D *)mem[0]));
        
        queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(fourth_width, fourth_height), cl::NullRange, NULL, &event);
        event.wait();*/
        
        inUnlock();
    }
    
private:
    
    void push_seq(cl::Image2D * top, cl::Image2D * bottom, const unsigned int &width, const unsigned int &height) {
		cl::Event event;
        push->setArg(0, *top);
        push->setArg(1, *bottom);
        queue->enqueueNDRangeKernel(*push, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
        event.wait();
    }
    
    void pull_seq(cl::Image2D * bottom, cl::Image2D * top, const unsigned int &bottom_width, const unsigned int &bottom_height, const unsigned int &top_width, const unsigned int &top_height) {

		cl::Event event;
		pull_copy->setArg(0, *bottom);
        pull_copy->setArg(1, *top);
        pull_copy->setArg(2, *top);
        queue->enqueueNDRangeKernel(*pull_copy, cl::NullRange, cl::NDRange(bottom_width, bottom_height), cl::NullRange, NULL, &event);
        event.wait();
        
        copy->setArg(0, *top);
        copy->setArg(1, *scratch);
        queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
        event.wait();
        
        pull_diag->setArg(0, *top);
        pull_diag->setArg(1, *scratch);
        queue->enqueueNDRangeKernel(*pull_diag, cl::NullRange, cl::NDRange(bottom_width, bottom_height), cl::NullRange, NULL, &event);
        event.wait();
        
        copy->setArg(0, *scratch);
        copy->setArg(1, *top);
        queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
        event.wait();
        
        pull_horiz->setArg(0, *scratch);
        pull_horiz->setArg(1, *top);
        queue->enqueueNDRangeKernel(*pull_horiz, cl::NullRange, cl::NDRange(top_width, top_height), cl::NullRange, NULL, &event);
        event.wait();
    }
    
    cl::CommandQueue * queue = nullptr;
    
    cl::Kernel * copy = nullptr;
    cl::Kernel * push = nullptr;
    cl::Kernel * pull = nullptr;
    
    cl::Kernel * pull_copy = nullptr;
    cl::Kernel * pull_diag = nullptr;
    cl::Kernel * pull_horiz = nullptr;
    
    cl::Image2D * first = nullptr, * first_copy = nullptr,
                * second = nullptr, * second_copy = nullptr,
                * third = nullptr, * third_copy = nullptr,
                * fourth = nullptr, * fourth_copy = nullptr,
                * fifth = nullptr, * fifth_copy = nullptr, * scratch = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
    
    unsigned int first_width, first_height;
    unsigned int second_width, second_height;
    unsigned int third_width, third_height;
    unsigned int fourth_width, fourth_height;
    unsigned int fifth_width, fifth_height;
    
};

#endif
