//
//  trayraceProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_trayraceProcessor_hpp
#define video_mush_trayraceProcessor_hpp

#include <Video Mush/opencl.hpp>
#include <Video Mush/fixedExposureProcess.hpp>
#include <Video Mush/guiExposureProcess.hpp>
#include <Video Mush/nullProcess.hpp>
#include <Video Mush/doubleBuffer.hpp>
#include <Video Mush/laplaceProcess.hpp>
#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"
#include "getDiscontinuities.hpp"
#include "trayraceRedraw.hpp"
#include "trayraceCompose.hpp"
#include <Video Mush/imageProcessor.hpp>
#include <thread>

class trayraceProcessor : public mush::imageProcessor {
public:
	trayraceProcessor(float gamma, float darken) :
	gamma(gamma),
	darken(darken) {
        	}
    
	~trayraceProcessor() {}
    
    void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
        profileInit();
        
        if (buffers.size() < 1) {
            putLog("No buffers at encoder");
        }

//        doublers.push_back(make_shared<mush::doubleBuffer>());
//        doublers[2]->init(std::dynamic_pointer_cast<mush::imageBuffer>(doublers[1]->getSecond()));
        
        // take the three prelim buffers first
        
        depthDoubler = make_shared<mush::doubleBuffer>();
        depthDoubler->init(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
        
        depthExposure = make_shared<fixedExposureProcess>(-8.0f);
        depthExposure->init(context, depthDoubler->getFirst());
        
        normalExposure = make_shared<fixedExposureProcess>(darken);
        normalExposure->init(context, buffers[2]);
        
        colourExposure = make_shared<fixedExposureProcess>(0);
        colourExposure->init(context, buffers[3]);
        
        colourDoubler = make_shared<mush::doubleBuffer>();
        colourDoubler->init(std::dynamic_pointer_cast<mush::imageBuffer>(colourExposure));
        
        // and send back the redrawmap
        
        discontinuities = make_shared<getDiscontinuities>();
        std::vector<std::shared_ptr<mush::ringBuffer>> motBuff;
        motBuff.push_back(colourDoubler->getFirst());
        motBuff.push_back(depthDoubler->getSecond());
        discontinuities->init(context, motBuff);
        
        redraw = make_shared<trayraceRedraw>();
        redraw->init(context, discontinuities);
        
        inputBuffer = buffers[0];
        
        mainExposure = make_shared<fixedExposureProcess>(darken);
        mainExposure->init(context, inputBuffer);
        
        mainDoubler = make_shared<mush::doubleBuffer>();
        mainDoubler->init(std::dynamic_pointer_cast<mush::imageBuffer>(mainExposure));
        
        compose = make_shared<trayraceCompose>();
        std::vector<std::shared_ptr<mush::ringBuffer>> composeBuff;
        composeBuff.push_back(mainDoubler->getFirst());
        composeBuff.push_back(redraw);
        composeBuff.push_back(colourDoubler->getSecond());
        compose->init(context, composeBuff);
        
        // then we can get the main image and start workin'
        

        laplace = make_shared<laplaceProcess>();
        laplace->init(context, depthExposure);
        
        edge = make_shared<edgeThreshold>();
        std::vector<std::shared_ptr<mush::ringBuffer>> edgeBuff;
        edgeBuff.push_back(mainDoubler->getSecond());
        edgeBuff.push_back(laplace);
        edge->init(context, edgeBuff);
        
        diffuse = make_shared<diffuseProcess>();
        diffuse->init(context, edge);

        _guiBuffers.push_back(compose);
//        _guiBuffers.push_back(depthExposure);
//        _guiBuffers.push_back(laplace);
//        _guiBuffers.push_back(edge);
        _guiBuffers.push_back(colourExposure);
        _guiBuffers.push_back(discontinuities);
        _guiBuffers.push_back(redraw);
//        _guiBuffers.push_back(colourExposure);
        _guiBuffers.push_back(mainExposure);
        
//        _guiBuffers.push_back(discontinuities);
        
        
        _nulls.push_back(diffuse);
        _nulls.push_back(normalExposure);
//        _nulls.push_back(colourExposure);
//        _nulls.push_back(redraw);
        
        
		queue = context->getQueue();
        
        //_guiBuffers.push_back(imageBuffer);
    }
    
	void doFrame() {
        profile.start();
        
        profile.inReadStart();
        profile.inReadStop();
        
        profile.writeToGPUStart();
        profile.writeToGPUStop();
        
        profile.executionStart();
        
        normalExposure->process();
        colourExposure->process();
        discontinuities->process();
        redraw->process();
        
        mainExposure->process();
        compose->process();
        depthExposure->process();
        laplace->process();
        edge->process();
        diffuse->process();
        
        profile.executionStop();
        
        profile.writeStart();
        for (auto img : _nulls) {
            img->outUnlock(); // removes the need for nullers; hacky hacky hack hack
        }
        profile.writeStop();
        
        profile.readFromGPUStart();
        profile.readFromGPUStop();
        
        profile.waitStart();
        profile.waitStop();
        
        profile.stop();
        profile.frameReport();
	}
    
	void profileInit() {
		profile.init();
	}
    
	void profileReport() {
		profile.totalReport();
	}
    
	static const std::vector<std::string> listKernels() {
		std::vector<std::string> kernels;
		return kernels;
	}
    
    std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() {
        std::vector<std::shared_ptr<mush::ringBuffer>> buffers;
        buffers.push_back(compose);
        return buffers;
    }
    
    std::vector<std::shared_ptr<mush::imageBuffer>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::imageBuffer>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void go() {
        while (inputBuffer->good()) {
			doFrame();
		}
        
        if (mainExposure != nullptr) {
            mainExposure->release();
        }
        
        if (edge != nullptr) {
            edge->release();
        }
        
        if (laplace != nullptr) {
            laplace->release();
        }
        
        if (depthExposure != nullptr) {
            depthExposure->release();
        }
        
        if (normalExposure != nullptr) {
            normalExposure->release();
        }
        
        if (colourExposure != nullptr) {
            colourExposure->release();
        }
        
        if (discontinuities != nullptr) {
            discontinuities->release();
        }
        
        if (redraw != nullptr) {
            redraw->release();
        }
        
        if (compose != nullptr) {
            compose->release();
        }
        
    }
                                  
    std::shared_ptr<trayraceRedraw> getRedraw() {
        return std::dynamic_pointer_cast<trayraceRedraw>(redraw);
    }
    
    //    std::shared_ptr<guiProcess> gui = nullptr;
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    shared_ptr <mush::imageProcess> laplace  = nullptr;
    shared_ptr <mush::imageProcess> edge = nullptr;
    shared_ptr <mush::imageProcess> diffuse = nullptr;
    
    shared_ptr <mush::imageProcess> mainExposure  = nullptr;
    shared_ptr <mush::imageProcess> depthExposure  = nullptr;
    shared_ptr <mush::imageProcess> normalExposure  = nullptr;
    shared_ptr <mush::imageProcess> colourExposure  = nullptr;
    
    shared_ptr <mush::integerMapProcess> discontinuities  = nullptr;
    shared_ptr <mush::integerMapProcess> redraw  = nullptr;
    shared_ptr <mush::imageProcess> compose  = nullptr;
    
    std::shared_ptr<mush::doubleBuffer> colourDoubler;
    std::shared_ptr<mush::doubleBuffer> depthDoubler;
    std::shared_ptr<mush::doubleBuffer> mainDoubler;
    
    std::vector<std::shared_ptr<mush::imageProcess>> _nulls;
    
    std::vector<std::shared_ptr<mush::imageBuffer>> _guiBuffers;
    
	float gamma;
	float darken;
    
	cl::Event event;
	cl::CommandQueue * queue;
    
	Profile profile;
};

#endif
