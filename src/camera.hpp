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
			: width(width), height(height) {
			point3d gaze(1, 0, 0);
			point3d up(0, 1, 0);

			yRotation yRotate(theta*(M_PI/180.0));
			zRotation zRotate(phi*(M_PI/180.0));

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
					cells.push_back(Cell(line3d(location, dir), i, j, width, height, u, v));
                    depths.push_back(0.0);
                    normals.push_back(point3d());
                    colourMap.push_back(Light::rgb());
				}
			}
		}

		~Camera() {

		}

		void snap(shared_ptr<Viewport> viewport,
                  shared_ptr<Viewport> depthMap,
                  shared_ptr<Viewport> normalMap,
                  shared_ptr<Viewport> colourMap,
			Shapes shapes,
			Lights lights) {
			int percent = 0;
			Profile profile;
			int64_t oldPercentile = 0;

			int64_t cellcount = cells.size();
			int64_t overflow = cellcount % 4;
			int64_t cells_per_thread = (cellcount - overflow) / 4;
            std::atomic_long count4;
            count4 = 0;

			int64_t start1 = 0,
				start2 = cells_per_thread,
				start3 = cells_per_thread * 2,
				start4 = cells_per_thread * 3;

			std::thread thread1(std::bind(Camera::subSnap, start1, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4)));
			std::thread thread2(std::bind(Camera::subSnap, start2, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4)));
			std::thread thread3(std::bind(Camera::subSnap, start3, cells_per_thread, viewport, depthMap, normalMap, colourMap, cells, shapes, lights, std::ref(count4)));
			
			profile.init();
			for (int64_t i = 0; i < (cells_per_thread+overflow); ++i) {
				profile.start();
                double depth;
                point3d normal;
                Light::rgb colour;
				viewport->put(cells[start4+i].fire4(shapes, lights, depth, normal, colour), start4+i);
                depthMap->put(Light::rgb(depth, depth, depth), start4+i);
                normalMap->put(Light::rgb(normal.x, normal.y, normal.z), start4+i);
                colourMap->put(colour, start4+i);
				++count4;
				
				if ((count4 - oldPercentile) >(width*height) / 100.0) {
					std::cout << "Processing pixel " << (count4) << " of " << width*height << " " << ++percent << "% (" << profile.total / 1e9 << "s elapsed) \r";
					oldPercentile = count4;
				}
				profile.stop();
			}
			thread1.join();
			thread2.join();
			thread3.join();
			
			std::cout << "Done in (" << profile.total / 1e9 << "s elapsed)                                              \r";
		}

		static void subSnap(int64_t start, int64_t num,
                            shared_ptr<Viewport> viewport,
                            shared_ptr<Viewport> depthMap,
                            shared_ptr<Viewport> normalMap,
                            shared_ptr<Viewport> colourMap,
                            const vector<Cell> &cells, Shapes shapes, Lights lights, std::atomic_long &count) {
            
			for (int64_t i = 0; i < num; ++i) {
                double depth;
                point3d normal;
                Light::rgb colour;
				viewport->put(cells[start+i].fire4(shapes, lights, depth, normal, colour), start+i);
                depthMap->put(Light::rgb(depth, depth, depth), start+i);
                normalMap->put(Light::rgb(normal.x, normal.y, normal.z), start+i);
                colourMap->put(colour, start+i);
				++count;
			}
		}

	private:
		double n;
		unsigned int width, height;
		vector<Cell> cells;
        Depths depths;
        Normals normals;
        Colours colourMap;
	};
}

#endif