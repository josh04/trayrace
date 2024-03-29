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
    class quitEventHandler;
}

class edgeThreshold;
class diffuseProcess;
class getDiscontinuities;
class trayraceRedraw;
class trayraceUpsample;
class trayraceCompose;

#include <Video Mush/opencl.hpp>
//#define _PROFILE 3
#include <Video Mush/profile.hpp>

#include <Video Mush/imageProcessor.hpp>
#include <thread>

class trayraceProcessor : public mush::imageProcessor {
public:
    trayraceProcessor(std::shared_ptr<mush::imageBuffer> viewport, std::shared_ptr<mush::imageBuffer> depthMap,
                      std::shared_ptr<mush::imageBuffer> normalMap, std::shared_ptr<mush::imageBuffer> colourMap ) : mush::imageProcessor(), _viewport(viewport), _depthMap(depthMap), _normalMap(normalMap), _colourMap(colourMap) {
        	}
    
	~trayraceProcessor() {}
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers);
    
	void process();
    
	static const std::vector<std::string> listKernels();
    
    std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers();
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers();
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers();
    
    void go();
                                  
    std::shared_ptr<trayraceRedraw> getRedraw();
    
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    shared_ptr <mush::imageBuffer> _viewport  = nullptr;
    shared_ptr <mush::imageBuffer> _depthMap  = nullptr;
    shared_ptr <mush::imageBuffer> _normalMap  = nullptr;
    shared_ptr <mush::imageBuffer> _colourMap  = nullptr;
    
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
    
    std::shared_ptr<mush::quitEventHandler> quitHandler = nullptr;
    
	cl::Event event;
	cl::CommandQueue * queue;
    
};

#endif
