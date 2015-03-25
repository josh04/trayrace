// trayrace 

#include <thread>
#include <memory>
#include <iostream>
#include <array>
#include <tiffio.h>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif
#include <Video Mush/ConfigStruct.hpp>
#include <Video Mush/exports.hpp>

#include <boost/thread.hpp>

#include "scene.hpp"
#include "viewport8bit.hpp"
#include "viewportFloat.hpp"
#include "tiffOutput.hpp"

#include "video-mush/trayraceProcessor.hpp"
#include "video-mush/trayraceRedraw.hpp"

using namespace tr;
using std::make_shared;


void runLog(std::atomic_int * over) {
	const uint64_t size = 4096;
	char output[size];
	while (!(*over)) {
		boost::this_thread::sleep_for(boost::chrono::duration<int64_t, boost::micro>(2));
		bool set = false;
		set = getLog(output, size);
		if (set) {
			std::cout << output << std::endl;
		}
	}
};

std::shared_ptr<trayraceProcessor> vmp = nullptr;

void doTrayRaceThread(SceneStruct sconfig, std::atomic<bool> * stop,
                      std::shared_ptr<ViewportFloat> viewport, std::shared_ptr<ViewportFloat> depthMap,
                      std::shared_ptr<ViewportFloat> normalMap, std::shared_ptr<ViewportFloat> colourMap) {
    
//    trayraceProcessor * vmpptr = vmp.get();
	Scene scene{};
	scene.init(&sconfig);
    
    
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
        
        depthMap->process();
        normalMap->process();
        colourMap->process();
        
        /*
        unsigned char * inptr = nullptr;
        
		// depth
        inptr = (unsigned char *) inputPtrs[1]->lockInput();
		if (inptr == nullptr) {break;}
		memcpy(inptr, depthMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[1]->unlockInput();
        
		// normals
        inptr = (unsigned char *) inputPtrs[2]->lockInput();
		if (inptr == nullptr) {break;}
		memcpy(inptr, normalMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[2]->unlockInput();
        
		// colour
        inptr = (unsigned char *) inputPtrs[3]->lockInput();
		if (inptr == nullptr) {break;}
		memcpy(inptr, colourMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[3]->unlockInput();
        */
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
        /*
		// view
		inptr = (unsigned char *) inputPtrs[0]->lockInput();
		if (inptr == nullptr) {break;}
		memcpy(inptr, viewport->ptr(), size * 4 * sconfig.width * sconfig.height);
		inputPtrs[0]->unlockInput();
        */
        
        viewport->process();
        
		sconfig.waistRotation -= 1;
		sconfig.CameraLocation = sconfig.CameraLocation * yRotation(-1.0*(M_PI/180.0));//+ point3d(0.5, 0, 0.5);
		scene.moveCamera(sconfig.CameraLocation, sconfig.waistRotation, sconfig.headTilt, sconfig.horizontalFov);
        scene.moveObject();
	}

    
    std::shared_ptr<mush::ringBuffer> redraw = vmp->getRedraw();
    redraw->kill();
    redraw = nullptr;
//    vmp = nullptr;
    /*
    for (int i = 0; i < inputPtrs.size(); ++i) {
        inputPtrs[i]->releaseInput();
    }
    */
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
//	config.processEngine = mush::processEngine::homegrown;
	// vlc output
	config.encodeEngine = mush::encodeEngine::none;
	config.outputEngine = mush::outputEngine::noOutput;
//    config.encodeEngine = mush::encodeEngine::x264;
//	config.outputEngine = mush::outputEngine::libavformatOutput;
	// Show the goHDR gui?
	config.show_gui = true;
	// Sim2 preview window
	config.sim2preview = false;
	// pixel format for external input
	config.inputConfig.input_pix_fmt = mush::input_pix_fmt::float_4channel;
    
    config.inputConfig.testMode = false;
    
#ifdef _WIN32
	config.resourceDir = "./";
	config.inputConfig.resourceDir = "./";
	config.outputConfig.outputPath = "./output";
#endif

#ifdef __APPLE__
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", @".", @"Video Mush.framework"];
	NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
	config.resourceDir = [resDir UTF8String];
	config.inputConfig.resourceDir =[resDir UTF8String];
	config.outputConfig.outputPath = "/Users/josh04/trayrace";
#endif

    config.outputConfig.outputName = "trayrace.mp4";
    
	// run function in thread
    videoMushInit(&config);
    
    // load viewport
    std::shared_ptr<ViewportFloat> viewport = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
    std::shared_ptr<ViewportFloat> depthMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
    std::shared_ptr<ViewportFloat> normalMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
    std::shared_ptr<ViewportFloat> colourMap = make_shared<ViewportFloat>(sconfig.width, sconfig.height);
    std::dynamic_pointer_cast<mush::imageProcess>(viewport)->init(videoMushGetContext(), nullptr);
    std::dynamic_pointer_cast<mush::imageProcess>(depthMap)->init(videoMushGetContext(), nullptr);
    std::dynamic_pointer_cast<mush::imageProcess>(normalMap)->init(videoMushGetContext(), nullptr);
    std::dynamic_pointer_cast<mush::imageProcess>(colourMap)->init(videoMushGetContext(), nullptr);
    
    vmp = make_shared<trayraceProcessor>(viewport, depthMap, normalMap, colourMap);
    
    std::thread * thread = new std::thread(&doTrayRaceThread, sconfig, &stop, viewport, depthMap, normalMap, colourMap);
    
    std::atomic<int> over(0);
	
//	std::thread logThread(&runLog, &over);

    videoMushExecute(vmp);
	over++;
    
    stop = true;
//    if (logThread->joinable()) {
//        logThread->join();
//    }
    if (thread->joinable()) {
        thread->join();
    }
    vmp = nullptr;
}
