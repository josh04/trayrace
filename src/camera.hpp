#ifndef CAMERA_HPP
#define CAMERA_HPP

#define _USE_MATH_DEFINES
#include <vector>
#include <iomanip>
#include <atomic>
#include <math.h>
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
                
            for (uint32 j = 0; j < height; ++j) {
                for (uint32 i = 0; i < width; ++i) {
                    cells.push_back(Cell(i, j, width, height));
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
            
			for (uint32 j = 0; j < height; ++j) {
				for (uint32 i = 0; i < width; ++i) {
					double ratio = static_cast<double>(height) / static_cast<double>(width);
					unit3d dir = gaze*n + v*ratio*(-1.0 + 2*((0.5 + j) / height)) + u*(-1.0 + 2*((0.5 + i) / width));
					cells[j*width +i].move(line3d(location, dir), u, v);
				}
			}
        }

		void snap(shared_ptr<Viewport> viewport,
                  shared_ptr<Viewport> depthMap,
                  shared_ptr<Viewport> normalMap,
                  shared_ptr<Viewport> colourMap,
			Shapes shapes,
			Lights lights) {

			int64_t cellcount = cells.size();
			int64_t overflow = cellcount % 4;
			int64_t cells_per_thread = (cellcount - overflow) / 4;
            std::atomic_long count4;
            count4 = 0;

			int64_t start1 = 0,
				start2 = cells_per_thread,
				start3 = cells_per_thread * 2,
				start4 = cells_per_thread * 3;

			std::thread thread1(&Camera::subSnap, this, start1, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4), false);
			std::thread thread2(&Camera::subSnap, this, start2, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4), false);
			std::thread thread3(&Camera::subSnap, this, start3, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4), false);
			
			profile.init();
            subSnap(start4, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4), true);
			thread1.join();
			thread2.join();
			thread3.join();
			
			std::cout << "Done in (" << profile.total / 1e9 << "s elapsed)                                              \r";
		}

		void subSnap(int64_t start, int64_t num,
                            shared_ptr<Viewport> viewport,
                            shared_ptr<Viewport> depthMap,
                            shared_ptr<Viewport> normalMap,
                            shared_ptr<Viewport> colourMap,
                            const vector<Cell> &cells, Shapes shapes, Lights lights, std::atomic_long &count, bool useProfile) {
            
			int64_t oldPercentile = 0;
			int percent = 0;
            
			for (int64_t i = 0; i < num; ++i) {
                if (useProfile) {
                    profile.start();
                }
                
                Cell::returnProps props;
				viewport->put(cells[start+i].fire4(shapes, lights, motion, props), start+i);
                depthMap->put(Light::rgb(props.depth, props.depth, props.depth), start+i);
                normalMap->put(Light::rgb(props.normal.x, props.normal.y, props.normal.z), start+i);
                colourMap->put(Light::rgb(props.movement.x, props.movement.y, props.movement.z), start+i);
				++count;
                
                if (useProfile) {
                    if ((count - oldPercentile) > (width*height) / 100.0) {
                        std::cout << "Processing pixel " << (count) << " of " << width*height << " " << ++percent << "% (" << profile.total / 1e9 << "s elapsed) \r";
                        oldPercentile = count;
                    }
                    profile.stop();
                }
			}
		}

	private:
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