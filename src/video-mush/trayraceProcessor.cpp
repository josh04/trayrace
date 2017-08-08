//
//  trayraceProcessor.cpp
//  trayrace
//
//  Created by Josh McNamee on 08/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#include <thread>

#include <Mush Core/opencl.hpp>
#include <Mush Core/mushMemoryToGpu.hpp>
#include <nullProcess.hpp>
#include <laplaceProcess.hpp>
#include <delayProcess.hpp>

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/frameStepper.hpp>
#include <Mush Core/SetThreadName.hpp>

#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"
#include "getDiscontinuities.hpp"
#include "trayraceRedraw.hpp"
#include "trayraceUpsample.hpp"
#include "trayraceCompose.hpp"

#include "trayraceProcessor.hpp"

void trayraceProcessor::init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
	if (buffers.size() < 4) {
		putLog("No buffers at encoder");
	}
	auto buf1 = std::dynamic_pointer_cast<mush::memoryToGpu>(buffers.begin()[0]);
	auto buf2 = std::dynamic_pointer_cast<mush::memoryToGpu>(buffers.begin()[1]);
	auto buf3 = std::dynamic_pointer_cast<mush::memoryToGpu>(buffers.begin()[2]);
	auto buf4 = std::dynamic_pointer_cast<mush::memoryToGpu>(buffers.begin()[3]);
	buf1->init(context, {});
	buf2->init(context, {});
	buf3->init(context, {});
	buf4->init(context, {});

	steppers.push_back(std::make_shared<mush::frameStepper>());

	// take the three prelim buffers first

	// and send back the redrawmap

	depthDelay = make_shared<mush::delayProcess>();
	depthDelay->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]));

	discontinuities = make_shared<getDiscontinuities>();
	discontinuities->init(context, {buffers.begin()[3], depthDelay});

	redraw = make_shared<trayraceRedraw>();
	redraw->init(context, discontinuities);

	inputBuffer = buffers.begin()[0];

	upsample = make_shared<trayraceUpsample>();
	upsample->init(context, {redraw, buffers.begin()[2], buffers.begin()[1], inputBuffer});

	compose = make_shared<trayraceCompose>();
	compose->init(context, {inputBuffer, discontinuities, buffers.begin()[3], upsample});

	// then we can get the main image and start workin'

	depthLaplace = make_shared<laplaceProcess>();
	depthLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]));

	depthEdgeSamples = make_shared<edgeThreshold>();
	depthEdgeSamples->init(context, { buffers.begin()[1], depthLaplace });

	depthReconstruction = make_shared<diffuseProcess>();
	depthReconstruction->init(context, depthEdgeSamples);

	motionLaplace = make_shared<laplaceProcess>();
	motionLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[3]));

	motionEdgeSamples = make_shared<edgeThreshold>();
	motionEdgeSamples->init(context, {buffers.begin()[3], motionLaplace});
    
    motionReconstruction = make_shared<diffuseProcess>();
    motionReconstruction->init(context, motionEdgeSamples);
    
    _guiBuffers.push_back(compose);
    _guiBuffers.push_back(upsample);
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[1]));
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[3]));
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers.begin()[2]));
    
    _guiBuffers.push_back(depthLaplace);
    _guiBuffers.push_back(depthEdgeSamples);
    _guiBuffers.push_back(depthReconstruction);
    
    _guiBuffers.push_back(motionLaplace);
    _guiBuffers.push_back(motionEdgeSamples);
    _guiBuffers.push_back(motionReconstruction);
    
    _guiBuffers.push_back(discontinuities);
    _guiBuffers.push_back(redraw);
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(inputBuffer));
    
    
    
    _nulls.push_back(depthReconstruction);
    _nulls.push_back(motionReconstruction);
    //		_nulls.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]));
    //        _nulls.push_back(colourExposure);
    //        _nulls.push_back(redraw);
    
    
    queue = context->getQueue();
	this->context = context;
    
    //_guiBuffers.push_back(imageBuffer);
}

void trayraceProcessor::process() {
    
    depthDelay->process();
    //        steppers[1]->process();
    steppers[0]->process();
    discontinuities->process();
    redraw->process();
    
    upsample->process();
    compose->process();
    depthLaplace->process();
    depthEdgeSamples->process();
    depthReconstruction->process();
    motionLaplace->process();
    motionEdgeSamples->process();
    motionReconstruction->process();
    
    for (auto img : _nulls) {
        img->outUnlock(); // removes the need for nullers; hacky hacky hack hack
    }

}

const std::vector<std::string> trayraceProcessor::listKernels() {
    std::vector<std::string> kernels;
    return kernels;
}

const std::vector<std::shared_ptr<mush::ringBuffer>> trayraceProcessor::getBuffers() const {
    std::vector<std::shared_ptr<mush::ringBuffer>> buffers;
    buffers.push_back(compose);
    return buffers;
}

std::vector<std::shared_ptr<mush::guiAccessible>> trayraceProcessor::getGuiBuffers() {
    auto buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

std::vector<std::shared_ptr<mush::frameStepper>> trayraceProcessor::getFrameSteppers() const {
    return steppers;
}

void trayraceProcessor::go() {
	SetThreadName("Reproj");
    while (inputBuffer->good()) {
        process();
    }
    
    
    if (depthEdgeSamples != nullptr) {
        depthEdgeSamples->release();
    }
    
    if (depthLaplace != nullptr) {
        depthLaplace->release();
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

std::shared_ptr<trayraceRedraw> trayraceProcessor::getRedraw() {
    return std::dynamic_pointer_cast<trayraceRedraw>(redraw);
}

