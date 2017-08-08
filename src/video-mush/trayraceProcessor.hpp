//
//  trayraceProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_trayraceProcessor_hpp
#define video_mush_trayraceProcessor_hpp

namespace mush {
    class imageProcess;
    class imageBuffer;
    class ringBuffer;
    class integerMapProcess;
    class frameStepper;
    class imageProcessor;
}

class edgeThreshold;
class diffuseProcess;
class getDiscontinuities;
class trayraceRedraw;
class trayraceUpsample;
class trayraceCompose;

#include <Mush Core/opencl.hpp>
//#define _PROFILE 3
#include <profile.hpp>

#include <Mush Core/imageProcessor.hpp>
#include <thread>

class trayraceProcessor : public mush::imageProcessor {
public:
	trayraceProcessor(float gamma, float darken) :
	gamma(gamma),
	darken(darken) {
        	}
    
	~trayraceProcessor() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
	void process() override;
    
	static const std::vector<std::string> listKernels();
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
    
    void go();
                                  
    std::shared_ptr<trayraceRedraw> getRedraw();
    
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    shared_ptr <mush::imageProcess> depthLaplace  = nullptr;
    shared_ptr <mush::imageProcess> depthEdgeSamples = nullptr;
    shared_ptr <mush::imageProcess> depthReconstruction = nullptr;

    shared_ptr <mush::imageProcess> motionLaplace  = nullptr;
    shared_ptr <mush::imageProcess> motionEdgeSamples = nullptr;
    shared_ptr <mush::imageProcess> motionReconstruction = nullptr;
    
    shared_ptr <mush::imageProcess> depthDelay  = nullptr;
    
    shared_ptr <mush::integerMapProcess> discontinuities  = nullptr;
    shared_ptr <mush::integerMapProcess> redraw  = nullptr;
    shared_ptr <mush::imageProcess> upsample  = nullptr;
    shared_ptr <mush::imageProcess> compose  = nullptr;
    
	std::vector<std::shared_ptr<mush::imageBuffer>> _nulls;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
    std::vector<std::shared_ptr<mush::frameStepper>> steppers;
    
	float gamma;
	float darken;
    
	cl::Event event;
	cl::CommandQueue * queue;
    
};

#endif
