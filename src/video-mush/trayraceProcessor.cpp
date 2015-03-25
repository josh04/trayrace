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
#include <Video Mush/quitEventHandler.hpp>
#include <Video Mush/exports.hpp>

#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"
#include "getDiscontinuities.hpp"
#include "trayraceRedraw.hpp"
#include "trayraceUpsample.hpp"
#include "trayraceCompose.hpp"

#include "trayraceProcessor.hpp"

void trayraceProcessor::init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    quitHandler = std::make_shared<mush::quitEventHandler>();
    videoMushAddEventHandler(quitHandler);
    
    steppers.push_back(std::make_shared<mush::frameStepper>());
    
    // take the three prelim buffers first
    
    // and send back the redrawmap
    
    depthDelay = make_shared<mush::delayProcess>();
    depthDelay->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(_depthMap));
    
    discontinuities = make_shared<getDiscontinuities>();
    std::vector<std::shared_ptr<mush::ringBuffer>> motBuff;
    motBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_colourMap));
    motBuff.push_back(depthDelay);
    discontinuities->init(context, motBuff);
    
    redraw = make_shared<trayraceRedraw>();
    redraw->init(context, discontinuities);
    
    inputBuffer = _viewport;
    
    upsample = make_shared<trayraceUpsample>();
    std::vector<std::shared_ptr<mush::ringBuffer>> upsampleBuff;
    upsampleBuff.push_back(redraw);
    upsampleBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_normalMap));
//    _normalMap->addRepeat();
    upsampleBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_depthMap));
    _depthMap->addRepeat();
    upsampleBuff.push_back(inputBuffer);
    upsample->init(context, upsampleBuff);
    
    compose = make_shared<trayraceCompose>();
    std::vector<std::shared_ptr<mush::ringBuffer>> composeBuff;
    inputBuffer->addRepeat();
    composeBuff.push_back(inputBuffer);
    discontinuities->addRepeat();
    composeBuff.push_back(discontinuities);
    _colourMap->addRepeat();
    composeBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_colourMap));
    composeBuff.push_back(upsample);
    compose->init(context, composeBuff);
    
    // then we can get the main image and start workin'
    
    depthLaplace = make_shared<laplaceProcess>();
    _depthMap->addRepeat();
    depthLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(_depthMap));
    
    depthEdgeSamples = make_shared<edgeThreshold>();
    std::vector<std::shared_ptr<mush::ringBuffer>> edgeBuff;
    _depthMap->addRepeat();
    edgeBuff.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_depthMap));
    edgeBuff.push_back(depthLaplace);
    depthEdgeSamples->init(context, edgeBuff);
    
    depthReconstruction = make_shared<diffuseProcess>();
    depthReconstruction->init(context, depthEdgeSamples);
    
    motionLaplace = make_shared<laplaceProcess>();
    _colourMap->addRepeat();
    motionLaplace->init(context, std::dynamic_pointer_cast<mush::imageBuffer>(_colourMap));
    
    motionEdgeSamples = make_shared<edgeThreshold>();
    std::vector<std::shared_ptr<mush::ringBuffer>> edgeBuff2;
    _colourMap->addRepeat();
    edgeBuff2.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_colourMap));
    edgeBuff2.push_back(motionLaplace);
    motionEdgeSamples->init(context, edgeBuff2);
    
    motionReconstruction = make_shared<diffuseProcess>();
    motionReconstruction->init(context, motionEdgeSamples);
    
    compose->setTagInGuiName("Full reconstruction");
    _guiBuffers.push_back(compose);
    upsample->setTagInGuiName("Spatially upsampled image");
    _guiBuffers.push_back(upsample);
    _guiBuffers.push_back(_depthMap);
    _guiBuffers.push_back(_colourMap);
    _guiBuffers.push_back(_normalMap);
    
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

std::vector<std::shared_ptr<mush::ringBuffer>> trayraceProcessor::getBuffers() {
    std::vector<std::shared_ptr<mush::ringBuffer>> buffers;
    buffers.push_back(compose);
    return buffers;
}

std::vector<std::shared_ptr<mush::guiAccessible>> trayraceProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

std::vector<std::shared_ptr<mush::frameStepper>> trayraceProcessor::getFrameSteppers() {
    return steppers;
}

void trayraceProcessor::go() {
    while (!quitHandler->getQuit()) {
        process();
    }
    
    if (_viewport != nullptr) {
        _viewport->release();
    }
    
    if (_depthMap != nullptr) {
        _depthMap->release();
    }
    
    if (_normalMap != nullptr) {
        _normalMap->release();
    }
    if (_colourMap != nullptr) {
        _colourMap->release();
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

