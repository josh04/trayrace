//
//  trayraceProcessor.cpp
//  trayrace
//
//  Created by Josh McNamee on 08/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#include <thread>

#include <Video Mush/opencl.hpp>
#include <Video Mush/nullProcess.hpp>
#include <Video Mush/laplaceProcess.hpp>
#include <Video Mush/delayProcess.hpp>

#include <Video Mush/imageProcessor.hpp>

#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"
#include "getDiscontinuities.hpp"
#include "trayraceRedraw.hpp"
#include "trayraceUpsample.hpp"
#include "trayraceCompose.hpp"

#include "trayraceProcessor.hpp"

void trayraceProcessor::init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
    profileInit();
    
    if (buffers.size() < 1) {
        putLog("No buffers at encoder");
    }
    
    steppers.push_back(std::make_shared<mush::frameStepper>());
    
    // take the three prelim buffers first
    
    // and send back the redrawmap
    
    depthDelay = make_shared<mush::delayProcess>();
    depthDelay->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
    
    discontinuities = make_shared<getDiscontinuities>();
    std::vector<std::shared_ptr<mush::ringBuffer>> motBuff;
    motBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]));
    motBuff.push_back(depthDelay);
    discontinuities->init(context, motBuff);
    
    redraw = make_shared<trayraceRedraw>();
    redraw->init(context, discontinuities);
    
    inputBuffer = buffers[0];
    
    upsample = make_shared<trayraceUpsample>();
    std::vector<std::shared_ptr<mush::ringBuffer>> upsampleBuff;
    upsampleBuff.push_back(redraw);
    upsampleBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]));
    buffers[1]->addRepeat();
    upsampleBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
    upsampleBuff.push_back(inputBuffer);
    upsample->init(context, upsampleBuff);
    
    compose = make_shared<trayraceCompose>();
    std::vector<std::shared_ptr<mush::ringBuffer>> composeBuff;
    inputBuffer->addRepeat();
    composeBuff.push_back(inputBuffer);
    discontinuities->addRepeat();
    composeBuff.push_back(discontinuities);
    buffers[3]->addRepeat();
    composeBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]));
    composeBuff.push_back(upsample);
    compose->init(context, composeBuff);
    
    // then we can get the main image and start workin'
    
    depthLaplace = make_shared<laplaceProcess>();
    buffers[1]->addRepeat();
    depthLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
    
    depthEdgeSamples = make_shared<edgeThreshold>();
    std::vector<std::shared_ptr<mush::ringBuffer>> edgeBuff;
    buffers[1]->addRepeat();
    edgeBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
    edgeBuff.push_back(depthLaplace);
    depthEdgeSamples->init(context, edgeBuff);
    
    depthReconstruction = make_shared<diffuseProcess>();
    depthReconstruction->init(context, depthEdgeSamples);
    
    motionLaplace = make_shared<laplaceProcess>();
    buffers[3]->addRepeat();
    motionLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]));
    
    motionEdgeSamples = make_shared<edgeThreshold>();
    std::vector<std::shared_ptr<mush::ringBuffer>> edgeBuff2;
    buffers[3]->addRepeat();
    edgeBuff2.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]));
    edgeBuff2.push_back(motionLaplace);
    motionEdgeSamples->init(context, edgeBuff2);
    
    motionReconstruction = make_shared<diffuseProcess>();
    motionReconstruction->init(context, motionEdgeSamples);
    
    _guiBuffers.push_back(compose);
    _guiBuffers.push_back(upsample);
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[1]));
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[3]));
    _guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(buffers[2]));
    
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
    
    //_guiBuffers.push_back(imageBuffer);
}

void trayraceProcessor::doFrame() {
    profile.start();
    
    profile.inReadStart();
    profile.inReadStop();
    
    profile.writeToGPUStart();
    profile.writeToGPUStop();
    
    profile.executionStart();
    
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

const std::vector<std::string> trayraceProcessor::listKernels() {
    std::vector<std::string> kernels;
    return kernels;
}

std::vector<std::shared_ptr<mush::ringBuffer>> trayraceProcessor::getBuffers() {
    std::vector<std::shared_ptr<mush::ringBuffer>> buffers;
    buffers.push_back(compose);
    return buffers;
}

std::vector<std::shared_ptr<mush::imageBuffer>> trayraceProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::imageBuffer>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

std::vector<std::shared_ptr<mush::frameStepper>> trayraceProcessor::getFrameSteppers() {
    return steppers;
}

void trayraceProcessor::go() {
    profileInit();
    while (inputBuffer->good()) {
        doFrame();
    }
    profileReport();
    
    
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

