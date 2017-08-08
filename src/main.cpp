// trayrace 

#include <thread>
#include <chrono>
#include <memory>
#include <iostream>
#include <array>
#include <tiffio.h>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif
#include <ConfigStruct.hpp>
#include <exports.hpp>

#include <Mush Core/mushMemoryToGpu.hpp>
#include <Mush Core/SetThreadName.hpp>

#include "scene.hpp"
#include "viewport8bit.hpp"
#include "viewportFloat.hpp"
#include "tiffOutput.hpp"

#include "video-mush/trayraceProcessor.hpp"
#include "video-mush/trayraceRedraw.hpp"

using namespace tr;
using std::make_shared;


void runLog(std::atomic<bool> * over) {
	SetThreadName("Log Thread");
	const uint64_t size = 4096;
	char output[size];
	while (!(*over)) {
		std::this_thread::sleep_for(std::chrono::duration<int64_t, std::micro>(2));
		bool set = false;
		set = getLog(output, size);
		if (set) {
			std::cout << output << std::endl;
		}
	}
};

void doTrayRaceThread(SceneStruct sconfig, std::atomic<bool> * stop, std::vector<std::shared_ptr<mush::memoryToGpu>> mems, std::shared_ptr<trayraceProcessor> trayraceVideoMush) {
	SetThreadName("Trayrace");
	Scene scene{};
	scene.init(&sconfig);

	auto mainImage = mems[0];
	auto depthImage = mems[1];
	auto geomImage = mems[2];
	auto motionImage = mems[3];
    
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
	
	mush::registerContainer<mush::ringBuffer> redraw_map = trayraceVideoMush->getRedraw()->getRedrawMap();

	// lock image, copy memory into place
	while (counter < 359 && !(*stop)) {
		++counter;
		scene.preSnap(depthMap, normalMap, colourMap);
        unsigned char * inptr = nullptr;
        
		// depth
		depthImage->set_ptr(depthMap->ptr());
		depthImage->process();
        
		// normals
		geomImage->set_ptr(normalMap->ptr());
		geomImage->process();
        
		// colour
		motionImage->set_ptr(colourMap->ptr());
		motionImage->process();
        
		auto redraw = trayraceVideoMush->getRedraw();
        uint8_t * redrawMap = nullptr;
        {
            redrawMap = (uint8_t *)redraw_map->outLock().get_pointer();
            if (firstRun) {
                redrawMap = nullptr;
                firstRun = false;
            }
            
            scene.snap(redrawMap, viewport);
			redraw_map->outUnlock();
        }
        
		// view
		mainImage->set_ptr(viewport->ptr());
		mainImage->process();
        
		sconfig.waistRotation -= 1;
		sconfig.CameraLocation = sconfig.CameraLocation * yRotation(-1.0*(M_PI/180.0));//+ point3d(0.5, 0, 0.5);
		scene.moveCamera(sconfig.CameraLocation, sconfig.waistRotation, sconfig.headTilt, sconfig.horizontalFov);
        scene.moveObject();
	}

    
    auto redraw = trayraceVideoMush->getRedraw();
    redraw->kill();
    redraw = nullptr;
    trayraceVideoMush = nullptr;

	mainImage->release();
	depthImage->release();
	geomImage->release();
	motionImage->release();
    
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
	config.inputConfig.inputEngine = mush::inputEngine::externalInput;
	// gohdr method
	//config.processEngine = mush::processEngine::homegrown;
	// vlc output
	config.outputConfig.encodeEngine = mush::encodeEngine::none;
	config.outputConfig.outputEngine = mush::outputEngine::noOutput;
	//config.encodeEngine = mush::encodeEngine::ffmpeg;
	//config.outputEngine = mush::outputEngine::libavformatOutput;
	// Show the goHDR gui?
	config.gui.show_gui = true;
	// Sim2 preview window
	config.gui.sim2preview = false;
	// pixel format for external input
	config.inputConfig.input_pix_fmt = mush::input_pix_fmt::float_4channel;
    
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

	auto mainImage = std::make_shared<mush::memoryToGpu>(sconfig.width, sconfig.height);
	auto depthImage = std::make_shared<mush::memoryToGpu>(sconfig.width, sconfig.height);
	auto geomImage = std::make_shared<mush::memoryToGpu>(sconfig.width, sconfig.height);
	auto motionImage = std::make_shared<mush::memoryToGpu>(sconfig.width, sconfig.height);
        
    std::shared_ptr<trayraceProcessor> vmp = make_shared<trayraceProcessor>(config.inputConfig.gammacorrect, config.inputConfig.darken);
	std::vector <std::shared_ptr<mush::memoryToGpu>> mems = { mainImage,depthImage,geomImage,motionImage };
	std::thread * thread = new std::thread(&doTrayRaceThread, sconfig, &stop, mems, vmp);
    
    std::thread * logThread = new std::thread(&runLog, &stop);
    
	videoMushUseExistingProcessor(vmp, { mainImage, depthImage, geomImage, motionImage });

	bool tr = true;
	bool * ptr = &tr;
    videoMushExecute(1, &ptr);
    
    stop = true;

    if (thread->joinable()) {
        thread->join();
    }
}
