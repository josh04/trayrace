#ifndef CAMERA_HPP
#define CAMERA_HPP

#define _USE_MATH_DEFINES

#include <vector>
#include <iomanip>
#include <atomic>
#include <math.h>

#include <Video Mush/imageBuffer.hpp>

#include "point3d.hpp"
#include "cell.hpp"
#include "viewport8bit.hpp"
#include "matrix3d.hpp"
#include "profile.hpp"

using std::vector;

namespace tr {
	typedef vector<double> Depths;
	typedef vector<point3d> Normals;
    typedef vector<Light::rgb> Colours;
}

namespace tr {
	class Camera {
	public:
		Camera(point3d location, double theta, double phi, double horizontalFOV, unsigned int width, unsigned int height)
			: location(location), width(width), height(height), horizontalFOV(horizontalFOV), yRotate(yRotation(theta*(M_PI/180.0))), zRotate(zRotation(phi*(M_PI/180.0))) {
                
            for (uint32 j = 0; j < height * _vSubsample; j = j+_vSubsample) {
                for (uint32 i = 0; i < width * _hSubsample; ++i) {
                    for (uint32 k = 0; k < _vSubsample; ++k) {
                        cells.push_back(Cell(i, j+k, width * _hSubsample, height * _vSubsample));
                    }
                }
            }
            move(location, theta, phi, horizontalFOV);
		}

		~Camera() {

		}
        
        void move(const point3d newLocation, const double newTheta, const double newPhi, const double changeFOV) {
            horizontalFOV = changeFOV;
            motion = motion3d(location, yRotate.inverse()*zRotate.inverse());
            location = newLocation;
            zRotate = zRotation(newPhi*(M_PI/180.0));
            yRotate = yRotation(newTheta*(M_PI/180.0));
            
            //motion = motion3d(move, (zRotation(addPhi*(M_PI/180.0))*yRotation(addTheta*(M_PI/180.0))).inverse());
            
			point3d gaze(1, 0, 0);
            point3d up(0, 1, 0);
            
			gaze = (gaze*zRotate)*yRotate;
			up = (up*zRotate)*yRotate;
			gaze = gaze.unit();
			up = up.unit();
            
			n = tan(horizontalFOV / 2.0*(M_PI / 180.0));
			up = up.unit();
			gaze = gaze.unit();
            
			unit3d w = -gaze / gaze.magnitude();
			unit3d u = up.cross(w);
			unit3d v = w.cross(u);
            
            double ratio = static_cast<double>(height) / static_cast<double>(width);
			for (uint32 j = 0; j < height * _vSubsample; j = j+_vSubsample) {
				for (uint32 i = 0; i < width * _hSubsample; ++i) {
                    
                    for (uint32 k = 0; k < _vSubsample; ++k) {
                        unit3d dir = gaze*n + v*ratio*(-1.0 + 2*((0.5 + j+k) / (height * _vSubsample))) + u*(-1.0 + 2*((0.5 + i) / (width * _hSubsample)));
                        cells[j*width*_hSubsample + i*_vSubsample + k].move(line3d(location, dir), u, v);
                    }

				}
			}
        }
        
        void preliminary_snap(shared_ptr<Viewport> depthMap,
                     shared_ptr<Viewport> normalMap,
                     shared_ptr<Viewport> colourMap,
                     Shapes shapes,
                     Lights lights) {
			int64_t viewportCellCount = cells.size() / (_hSubsample * _vSubsample);
			int64_t overflow = viewportCellCount % _numThreads;
			int64_t cells_per_thread = (viewportCellCount - overflow) / _numThreads;
            std::atomic_long count4;
            count4 = 0;
            
            std::vector<std::thread> threads;
            
            for (uint32_t i = 0; i < _numThreads-1; ++i) {
                threads.push_back(std::thread(&Camera::preliminary_subSnap, this, cells_per_thread*i, cells_per_thread, depthMap, normalMap, colourMap, std::ref(cells), shapes, lights, std::ref(count4), false));
            }
			
			profile.init();
            preliminary_subSnap(cells_per_thread*(_numThreads-1), cells_per_thread+overflow, depthMap, normalMap, colourMap, std::ref(cells), shapes, lights, std::ref(count4), true);
            
            for (uint32_t i = 0; i < threads.size(); ++i) {
                threads[i].join();
            }
			
			std::cout << "Done in (" << profile.total / 1e9 << "s elapsed)                                              \r";
        }

		void final_snap(shared_ptr<Viewport> viewport, Shapes shapes, Lights lights) {
			int64_t viewportCellCount = cells.size() / (_hSubsample * _vSubsample);
			int64_t overflow = viewportCellCount % _numThreads;
			int64_t cells_per_thread = (viewportCellCount - overflow) / _numThreads;
            std::atomic_long count4;
            count4 = 0;
            
            std::vector<std::thread> threads;
            
            for (uint32_t i = 0; i < _numThreads-1; ++i) {
                threads.push_back(std::thread(&Camera::final_subSnap, this, cells_per_thread*i, cells_per_thread, viewport, std::ref(cells), shapes, lights, std::ref(count4), false));
            }
			
			profile.init();
            final_subSnap(cells_per_thread*(_numThreads-1), cells_per_thread+overflow, viewport, std::ref(cells), shapes, lights, std::ref(count4), true);
            
            for (uint32_t i = 0; i < threads.size(); ++i) {
                threads[i].join();
            }
			
			std::cout << "Done in (" << profile.total / 1e9 << "s elapsed)                                              \r";
		}
        
        void activateAll() {
            for (int64_t i = 0; i < cells.size(); ++i) {
                cells[i].activate();
            }
        }
        
        void activateMap(uint8_t const * map) {
            for (int64_t i = 0; i < cells.size()/(_hSubsample*_vSubsample); ++i) {
                if (map[i] == 1) {
                    for (int k = 0; k < (_hSubsample*_vSubsample); ++k) {
                        cells[i*(_hSubsample*_vSubsample)+k].activate();
                    }
                } else {
                    for (int k = 0; k < (_hSubsample*_vSubsample); ++k) {
                        cells[i*(_hSubsample*_vSubsample)+k].deactivate();
                    }

                }
            }
        }
        
	private:
        
		void preliminary_subSnap(int64_t start, int64_t num,
                            shared_ptr<Viewport> depthMap,
                            shared_ptr<Viewport> normalMap,
                            shared_ptr<Viewport> colourMap,
                            vector<Cell> &cells, Shapes shapes, Lights lights, std::atomic_long &count, bool useProfile) {
            
			int64_t oldPercentile = 0;
			uint32_t percent = 0;
            
			for (int64_t i = 0; i < num; ++i) {
                if (useProfile) {
                    profileStart();
                }
                
                Light::rgb colour(0);
                Cell::returnProps props;
                
                for (int k = 0; k < _hSubsample * _vSubsample; ++k) {
                    if (cells[(start+i) * _hSubsample * _vSubsample + k].isActive()) {
                        Cell::returnProps tempProps;
                        colour += cells[(start+i) * _hSubsample * _vSubsample + k].preliminary_fire(shapes, lights, motion, tempProps);
                        props.depth += tempProps.depth;
                        props.normal += tempProps.normal;
                        props.movement += tempProps.movement;
                    }
                }
                //colour = colour / (double)(_hSubsample * _vSubsample);
                props.depth = props.depth / (double)(_hSubsample * _vSubsample);
                props.normal = props.normal / (double)(_hSubsample * _vSubsample);
                props.movement = props.movement / (double)(_hSubsample * _vSubsample);
                
                depthMap->put(Light::rgb(props.depth, props.depth, props.depth), start+i);
                normalMap->put(Light::rgb(props.normal.x, props.normal.y, props.normal.z), start+i);
                colourMap->put(Light::rgb(props.movement.x, props.movement.y, props.movement.z), start+i);
                
				++count;
                
                if (useProfile) {
                    profileStop(count, oldPercentile, percent);
                }
			}
		}
        
		void final_subSnap(int64_t start, int64_t num, shared_ptr<Viewport> viewport,
                     vector<Cell> &cells, Shapes shapes, Lights lights, std::atomic_long &count, bool useProfile) {
            
			int64_t oldPercentile = 0;
			uint32_t percent = 0;
            
			for (int64_t i = 0; i < num; ++i) {
                if (useProfile) {
                    profileStart();
                }
                
                Light::rgb colour(0);
                Cell::returnProps props;
                
                for (int k = 0; k < _hSubsample * _vSubsample; ++k) {
                    if (cells[(start+i) * _hSubsample * _vSubsample + k].isActive()) {
                        Cell::returnProps tempProps;
                        colour += cells[(start+i) * _hSubsample * _vSubsample + k].final_fire(shapes, lights, motion, tempProps);
                    }
                }
                colour = colour / (double)(_hSubsample * _vSubsample);
                
				viewport->put(colour, start+i);
                
				++count;
                
                if (useProfile) {
                    profileStop(count, oldPercentile, percent);
                }
			}
		}
        
        void profileStart() {
            profile.start();
        }
        
        void profileStop(std::atomic_long &count, int64_t &oldPercentile, uint32_t &percent) {
            if ((count - oldPercentile) > (width*height) / 100.0) {
                std::cout << "Processing pixel " << (count) << " of " << width*height << " " << ++percent << "% (" << profile.total / 1e9 << "s elapsed) \r";
                oldPercentile = count;
            }
            profile.stop();
        }
        
        const static uint32_t _hSubsample = 2, _vSubsample = 2;
        const static uint32_t _numThreads = 8;
        
        uint8_t const * map = nullptr;
        
		double n = 0;
        double horizontalFOV = 0;
        point3d location;
		const unsigned int width = 0, height = 0;
		vector<Cell> cells;
        
        matrix3d yRotate;
        matrix3d zRotate;
        Profile profile;
                                           
        motion3d motion;
	};
}

#endif