// trayrace 

#include <thread>
#include <memory>
#include <iostream>
#include <tiffio.h>

#import <Cocoa/Cocoa.h>

#include <Video Mush/ConfigStruct.hpp>
#include <Video Mush/exports.hpp>

#include "scene.hpp"
#include "viewport8bit.hpp"
#include "viewportFloat.hpp"
#include "tiffOutput.hpp"

using namespace tr;
using std::make_shared;

void doThread(unsigned int width, unsigned int height) {

	// initialise config struct
	hdr::config config;
	config.defaults();

	// width and height
	config.inputConfig.inputWidth = width;
	config.inputConfig.inputHeight = height;
	config.width = width;
	config.height = height;
	// external input
	config.inputEngine = hdr::inputEngine::externalInput;
	// gohdr method
	config.processEngine = hdr::processEngine::trayrace;
	// vlc output
	config.encodeEngine = hdr::encodeEngine::ffmpeg;
	config.outputEngine = hdr::outputEngine::libavformatOutput;
	// Show the goHDR gui?
	config.show_gui = true;
	// Sim2 preview window
	config.sim2preview = false;
	// pixel format for external input
	config.inputConfig.input_pix_fmt = hdr::input_pix_fmt::float_4channel;
    
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", @".", @"Video Mush.framework"];
	NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
	config.resourceDir =[resDir UTF8String];
    config.inputConfig.resourceDir =[resDir UTF8String];
    
	config.filename = "testRender_1080_";
    config.outputConfig.outputPath = "/Users/josh04/trayrace";
    config.outputConfig.outputName = "trayrace.mp4";
    
	// run function in thread
	hdrRunExternalInput(&config);
}

void doTrayRaceThread(SceneStruct sconfig, std::atomic<bool> * stop) {
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
	// lock image, copy memory into place
	while (counter < 360 && !(*stop)) {
		++counter;
		scene.snap(viewport, depthMap, normalMap, colourMap);
        
		// view
		unsigned char * in = (unsigned char *) lockInput();
		if (in == nullptr) {break;}
		memcpy(in, viewport->ptr(), size * 4 * sconfig.width * sconfig.height);
		unlockInput();
        
		// depth
        in = (unsigned char *) lockInput();
		if (in == nullptr) {break;}
		memcpy(in, depthMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		unlockInput();
        
		// normals
        in = (unsigned char *) lockInput();
		if (in == nullptr) {break;}
		memcpy(in, normalMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		unlockInput();
        
		// colour
        in = (unsigned char *) lockInput();
		if (in == nullptr) {break;}
		memcpy(in, colourMap->ptr(), size * 4 * sconfig.width * sconfig.height);
		unlockInput();
        
		sconfig.waistRotation -= 1;
		sconfig.CameraLocation = sconfig.CameraLocation * yRotation(-1.0*(M_PI/180.0));//+ point3d(0.5, 0, 0.5);
		scene.moveCamera(sconfig.CameraLocation, sconfig.waistRotation,
                        sconfig.headTilt, sconfig.horizontalFov, sconfig.width, sconfig.height);
        scene.moveObject();
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
	std::thread * thread = new std::thread(&doTrayRaceThread, sconfig, &stop);
    
    doThread(sconfig.width, sconfig.height);
    stop = true;
    thread->join();
}
