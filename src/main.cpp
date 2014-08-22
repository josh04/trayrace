// trayrace 

#include <thread>
#include <memory>
#include <iostream>
#include <array>
#include <tiffio.h>

#import <Cocoa/Cocoa.h>

#include <Video Mush/ConfigStruct.hpp>
#include <Video Mush/exports.hpp>

#include "scene.hpp"
#include "viewport8bit.hpp"
#include "viewportFloat.hpp"
#include "tiffOutput.hpp"

#include "video-mush/trayraceProcessor.hpp"

using namespace tr;
using std::make_shared;

void doTrayRaceThread(SceneStruct sconfig, std::atomic<bool> * stop, std::vector<std::shared_ptr<inputMethods>> inputPtrs, std::shared_ptr<trayraceProcessor> vmp) {
	Scene scene{};
	scene.init(&sconfig);
    
	// load viewport
	std::shared_ptr<ViewportFloat> viewport = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
	std::shared_ptr<ViewportFloat> depthMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
	std::shared_ptr<ViewportFloat> normalMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
	std::shared_ptr<ViewportFloat> colourMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
    
	// fire the rays from the viewport to the scene
	//	- inc. intersection code
    //	scene.snap(viewport);

	int size = 4;
	point3d vec(1, 1, 1);
	point3d vec2 = vec * yRotation(20.0*(M_PI / 180.0));
	point3d vec3 = vec * yRotation(10.0*(M_PI / 180.0)) * yRotation(10.0*(M_PI / 180.0));
    
	int counter = 0;
    bool firstRun = true;
	// lock image, copy memory into place
	while (counter < 359 && !(*stop)) {
		++counter;
		scene.preSnap(depthMap, normalMap, colourMap);
        unsigned char * in = nullptr;
        
		// depth
        in = (unsigned char *) inputPtrs[1]->lockInput();
		if (in == nullptr) {break;}
		memcpy(in, depthMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[1]->unlockInput();
        
		// normals
        in = (unsigned char *) inputPtrs[2]->lockInput();
		if (in == nullptr) {break;}
		memcpy(in, normalMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[2]->unlockInput();
        
		// colour
        in = (unsigned char *) inputPtrs[3]->lockInput();
		if (in == nullptr) {break;}
		memcpy(in, colourMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[3]->unlockInput();
        
        auto redraw = vmp->getRedraw();
        uint8_t * redrawMap = nullptr;
        {
            redrawMap = (uint8_t *)redraw->getRedrawMap().outLock();
            if (firstRun) {
                redrawMap = nullptr;
                firstRun = false;
            }
            
            scene.snap(redrawMap, viewport);
            redraw->getRedrawMap().outUnlock();
        }
        
		// view
		in = (unsigned char *) inputPtrs[0]->lockInput();
		if (in == nullptr) {break;}
		memcpy(in, viewport->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[0]->unlockInput();
        
		sconfig.waistRotation -= 1;
		sconfig.CameraLocation = sconfig.CameraLocation * yRotation(-1.0*(M_PI/180.0));//+ point3d(0.5, 0, 0.5);
		scene.moveCamera(sconfig.CameraLocation, sconfig.waistRotation, sconfig.headTilt, sconfig.horizontalFov);
        scene.moveObject();
	}
    
    std::shared_ptr<mush::imageBuffer> redraw = vmp->getRedraw();
    redraw->kill();
    redraw = nullptr;
    vmp = nullptr;
    
    for (int i = 0; i < inputPtrs.size(); ++i) {
        inputPtrs[i]->releaseInput();
    }
    
	// done!
}

int main(int argc, char** argv)
{
	// load scene data
	SceneStruct sconfig;
	sconfig.defaults();
	//	- list of objects
    std::atomic<bool> stop;
    stop = false;
    
    
	// initialise config struct
	mush::config config;
	config.defaults();
    
	// width and height
	config.inputConfig.inputWidth = sconfig.width;
	config.inputConfig.inputHeight = sconfig.height;
	// external input
	config.inputEngine = mush::inputEngine::externalInput;
	// gohdr method
	config.processEngine = mush::processEngine::homegrown;
	// vlc output
	//config.encodeEngine = mush::encodeEngine::none;
	//config.outputEngine = mush::outputEngine::noOutput;
	config.encodeEngine = mush::encodeEngine::ffmpeg;
	config.outputEngine = mush::outputEngine::libavformatOutput;
	// Show the goHDR gui?
	config.show_gui = true;
	// Sim2 preview window
	config.sim2preview = false;
	// pixel format for external input
	config.inputConfig.input_pix_fmt = mush::input_pix_fmt::float_4channel;
    
    config.inputConfig.testMode = false;
    
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", @".", @"Video Mush.framework"];
	NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
	config.resourceDir = [resDir UTF8String];
    config.inputConfig.resourceDir =[resDir UTF8String];
    
    config.outputConfig.outputPath = "/Users/josh04/trayrace";
    config.outputConfig.outputName = "trayrace.mp4";
    
	// run function in thread
    videoMushInit(&config);
    
    std::vector<std::shared_ptr<inputMethods>> inputPtrs;
    
    inputPtrs.push_back(videoMushAddInput());
    inputPtrs.push_back(videoMushAddInput());
    inputPtrs.push_back(videoMushAddInput());
    inputPtrs.push_back(videoMushAddInput());
    
    std::shared_ptr<trayraceProcessor> vmp = make_shared<trayraceProcessor>(config.gamma, config.darken);
    
	std::thread * thread = new std::thread(&doTrayRaceThread, sconfig, &stop, inputPtrs, vmp);
    
    videoMushExecute(vmp);
    
    stop = true;
    thread->join();
}
