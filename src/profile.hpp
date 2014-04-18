//
//  profile.hpp
//  compressor
//
//  Created by Jonathan Hatchett on 25/07/2012.
//  Copyright (c) 2012 goHDR. All rights reserved.
//

#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <stdio.h>
#include <iostream>
#include <boost/timer/timer.hpp>

#ifndef _PROFILE
#define _PROFILE 1
#endif

class Profile {
public:
#if _PROFILE > 0
	boost::timer::cpu_timer profileTotal;
	unsigned int frames;
	boost::timer::nanosecond_type total;
#endif
#if _PROFILE > 2
	boost::timer::cpu_timer profileReadFromPipe, 
							 profileWriteToPipe, 
							 profileWriteToGPU,
							 profileReadFromGPU, 
							 profileExecute,
							 profileWait;
#endif	
	
	inline void init() {
#if _PROFILE > 0
		frames = 0;
		total = 0;
#endif
#if _PROFILE > 1
		fprintf(stdout, "%12s %12s ", "Frame No:", "Frame Time: |");
#endif
#if _PROFILE > 2
		fprintf(stdout, "%12s %12s %12s %12s %12s %12s |", "Read:", "Write GPU:", "Execute", "Read GPU:", "Write:", "Wait");
#endif
#if _PROFILE > 1
		fprintf(stdout, "\n");
#endif
	}
	
	inline void frameReport() {
#if _PROFILE > 1
		fprintf(stdout, "%12i %11fs  |", frames, profileTotal.elapsed().wall / 1000000000.0);
#endif
#if _PROFILE > 2
		fprintf(stdout, "%11fs %11fs %11fs %11fs %11fs %11fs |",
			   	profileReadFromPipe.elapsed().wall / 1000000000.0,
			   	profileWriteToGPU.elapsed().wall / 1000000000.0,
			   	profileExecute.elapsed().wall / 1000000000.0,
			   	profileReadFromGPU.elapsed().wall / 1000000000.0,
			   	profileWriteToPipe.elapsed().wall / 1000000000.0,
				profileWait.elapsed().wall / 1000000000.0);
#endif
#if _PROFILE > 1
		fprintf(stdout, "\n");
#endif
	}
	
	inline void totalReport() {
#if _PROFILE > 0
		std::cout << std::endl;
		std::cout << "Total frame time: " << (total / 1000000000.0) << "s" << std::endl;
		std::cout << "Average frame time: " << ((total / (double)frames) / 1000000000.0) << "s" << std::endl;
		std::cout << "Average FPS: " << (1.0/((total / (double)frames) / 1000000000.0)) << std::endl;
#endif
	}
	
	inline void start() {
#if _PROFILE > 0
		profileTotal.start();
#endif
	}
	
	inline void stop() {
#if _PROFILE > 0
		profileTotal.stop();
		total += profileTotal.elapsed().wall;
		frames++;
#endif
	}
	
	inline void inReadStart() {
#if _PROFILE > 2
		profileReadFromPipe.start();
#endif
	}
	
	inline void inReadStop() {
#if _PROFILE > 2
		profileReadFromPipe.stop();
#endif
	}
	
	inline void executionStart() {
#if _PROFILE > 2
		profileExecute.start();
#endif
	}
	
	inline void executionStop() {
#if _PROFILE > 2
		profileExecute.stop();
#endif
	}
	
	inline void writeToGPUStart() {
#if _PROFILE > 2
		profileWriteToGPU.start();
#endif
	}
	
	inline void writeToGPUStop() {
#if _PROFILE > 2
		profileWriteToGPU.stop();
#endif
	}
	
	inline void waitStart() {
#if _PROFILE > 2
		profileWait.start();
#endif
	}
	
	inline void waitStop() {
#if _PROFILE > 2
		profileWait.stop();
#endif
	}
	
	inline void readFromGPUStart() {
#if _PROFILE > 2
		profileReadFromGPU.start();
#endif		
	}
	
	inline void readFromGPUStop() {
#if _PROFILE > 2
		profileReadFromGPU.stop();
#endif		
	}
	
	inline void writeStart() {
#if _PROFILE > 2
		profileWriteToPipe.start();
#endif		
	}
	
	inline void writeStop() {
#if _PROFILE > 2
		profileWriteToPipe.stop();
#endif		
	}
};

#endif
