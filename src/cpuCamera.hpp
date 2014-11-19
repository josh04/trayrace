#ifndef CPUCAMERA_HPP
#define CPUCAMERA_HPP

#include "camera.hpp"

using std::vector;

namespace tr {
	typedef vector<double> Depths;
	typedef vector<point3d> Normals;
	typedef vector<Light::rgb> Colours;
}

namespace tr {
	class cpuCamera : public Camera {
	public:
		cpuCamera(point3d location, double theta, double phi, double horizontalFOV, unsigned int width, unsigned int height)
			: Camera(), location(location), width(width), height(height), horizontalFOV(horizontalFOV), percentBoundary((width*height) / 100.0) {

			cells.reserve(height * width * _hSubsample * _vSubsample);
			for (uint32 j = 0; j < height * _vSubsample; j = j + _vSubsample) {
				for (uint32 i = 0; i < width * _hSubsample; ++i) {
					for (uint32 k = 0; k < _vSubsample; ++k) {
						cells.push_back(Cell(i, j + k, width * _hSubsample, height * _vSubsample));
					}
				}
			}
			move(location, theta, phi, horizontalFOV);
		}

		~cpuCamera() {

		}

		void move(const point3d newLocation, const double newTheta, const double newPhi, const double changeFOV) {
			horizontalFOV = changeFOV;
			motion = motion3d(location, yRotate.inverse()*zRotate.inverse());
			location = newLocation;
			zRotate = zRotation(newPhi*(M_PI / 180.0));
			yRotate = yRotation(newTheta*(M_PI / 180.0));

			//motion = motion3d(move, (zRotation(addPhi*(M_PI/180.0))*yRotation(addTheta*(M_PI/180.0))).inverse());

			const unit3d gaze(point3d(1, 0, 0) * zRotate * yRotate);
			const unit3d up(point3d(0, 1, 0) * zRotate * yRotate);

			n = tan(horizontalFOV / 2.0*(M_PI / 180.0));

			const unit3d w = -gaze;
			const unit3d u = up.cross(w);
			const unit3d v = w.cross(u);

			const int64_t subHeight = height *_vSubsample;
			const int64_t subWidth = width * _hSubsample;
			const point3d gazen = gaze*n;

			const point3d vratio = v * static_cast<double>(height) / static_cast<double>(width);

			for (uint32 j = 0; j < subHeight; j = j + _vSubsample) {
				for (uint32 i = 0; i < subWidth; ++i) {
					for (uint32 k = 0; k < _vSubsample; ++k) {
						unit3d dir = gazen + vratio*(-1.0 + 2.0*((0.5 + j + k) / (subHeight))) + u*(-1.0 + 2.0*((0.5 + i) / (subWidth)));
						cells[j*subWidth + i*_vSubsample + k].move(line3d(location, dir), u, v, horizontalFOV);
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
			int64_t count4 = 0;

			std::vector<std::thread> threads;

			for (uint32_t i = 0; i < _numThreads - 1; ++i) {
				threads.push_back(std::thread(&cpuCamera::preliminary_subSnap, this, cells_per_thread*i, cells_per_thread, depthMap, normalMap, colourMap, std::ref(cells), std::ref(shapes), std::ref(lights), std::ref(count4), false));
			}

			profile.init();
			preliminary_subSnap(cells_per_thread*(_numThreads - 1), cells_per_thread + overflow, depthMap, normalMap, colourMap, std::ref(cells), shapes, lights, std::ref(count4), true);

			for (uint32_t i = 0; i < threads.size(); ++i) {
				threads[i].join();
			}

			std::cout << "Done in (" << profile.total / 1e9 << "s elapsed)                                              \r";
		}

		void final_snap(shared_ptr<Viewport> viewport, Shapes shapes, Lights lights) {
			int64_t viewportCellCount = cells.size() / (_hSubsample * _vSubsample);
			int64_t overflow = viewportCellCount % _numThreads;
			int64_t cells_per_thread = (viewportCellCount - overflow) / _numThreads;
			int64_t count4 = 0;

			std::vector<std::thread> threads;

			for (uint32_t i = 0; i < _numThreads - 1; ++i) {
				threads.push_back(std::thread(&cpuCamera::final_subSnap, this, cells_per_thread*i, cells_per_thread, viewport, std::ref(cells), std::ref(shapes), std::ref(lights), std::ref(count4), false));
			}

			profile.init();
			final_subSnap(cells_per_thread*(_numThreads - 1), cells_per_thread + overflow, viewport, std::ref(cells), shapes, lights, std::ref(count4), true);

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
			const int subs = _hSubsample*_vSubsample;
			const int64_t size = cells.size() / (subs);
			for (int64_t i = 0; i < size; ++i) {
				if (map[i] == 1) {
					for (int k = 0; k < (subs); ++k) {
						cells[i*(subs)+k].activate();
					}
				} else {
					for (int k = 0; k < (subs); ++k) {
						cells[i*(subs)+k].deactivate();
					}

				}
			}
		}

	private:

		void preliminary_subSnap(int64_t start, int64_t num,
			shared_ptr<Viewport> depthMap,
			shared_ptr<Viewport> normalMap,
			shared_ptr<Viewport> colourMap,
			vector<Cell> &cells, Shapes &shapes, Lights &lights, int64_t &count, bool useProfile) {

			int64_t oldPercentile = 0;
			uint32_t percent = 0;

			for (int64_t i = 0; i < num; ++i) {

				Light::rgb colour(0);
				Cell::returnProps props;

				for (int k = 0; k < _hSubsample * _vSubsample; ++k) {
					if (cells[(start + i) * _hSubsample * _vSubsample + k].isActive()) {
						Cell::returnProps tempProps;
						colour += cells[(start + i) * _hSubsample * _vSubsample + k].preliminary_fire(std::ref(shapes), std::ref(lights), motion, tempProps);
						props.depth += tempProps.depth;
						props.normal += tempProps.normal;
						tempProps.movement.x = tempProps.movement.x / _hSubsample;
						tempProps.movement.y = tempProps.movement.y / _vSubsample;
						props.movement += tempProps.movement;
					}
				}
				//colour = colour / (double)(_hSubsample * _vSubsample);
				props.depth = props.depth / (double)(_hSubsample * _vSubsample);
				props.normal = unit3d(props.normal / (double)(_hSubsample * _vSubsample));
				props.movement = props.movement / (double)(_hSubsample * _vSubsample);
				//                props.movement.x = x - props.movement.x;
				//                props.movement.y = y - props.movement.y;

				depthMap->put(Light::rgb(props.depth, props.depth, props.depth), start + i);
				normalMap->put(Light::rgb(props.normal.x, props.normal.y, props.normal.z), start + i);
				colourMap->put(Light::rgb(props.movement.x, props.movement.y, props.movement.z), start + i);


				if (useProfile) {
					++count;
					profileStop(count, oldPercentile, percent);
				}
			}

			if (useProfile) {
				profile.stop();
			}
		}

		void final_subSnap(int64_t start, int64_t num, shared_ptr<Viewport> viewport,
			vector<Cell> &cells, Shapes &shapes, Lights &lights, int64_t &count, bool useProfile) {

			int64_t oldPercentile = 0;
			uint32_t percent = 0;

			for (int64_t i = 0; i < num; ++i) {

				Light::rgb colour(0);
				Cell::returnProps props;

				for (int k = 0; k < _hSubsample * _vSubsample; ++k) {
					if (cells[(start + i) * _hSubsample * _vSubsample + k].isActive()) {
						Cell::returnProps tempProps;
						colour += cells[(start + i) * _hSubsample * _vSubsample + k].final_fire(std::ref(shapes), std::ref(lights), motion, tempProps);
					}
				}
				colour = colour / (double)(_hSubsample * _vSubsample);

				viewport->put(colour, start + i);

				if (useProfile) {
					++count;
					profileStop(count, oldPercentile, percent);
				}
			}

			if (useProfile) {
				profile.stop();
			}
		}

		void profileStop(int64_t &count, int64_t &oldPercentile, uint32_t &percent) {
			if ((count - oldPercentile) > percentBoundary) {
				profile.stop();
				std::cout << "Processing pixel " << (count * _numThreads) << " of " << width*height << " " << ++percent * _numThreads << "% (" << profile.total / 1e9 << "s elapsed) \r";
				oldPercentile = count;
				profile.start();
			}
		}


		const static uint32_t _hSubsample = 2, _vSubsample = 2;

#ifdef __APPLE__
		const static uint32_t _numThreads = 8;
#endif
#ifdef _WIN32
		const static uint32_t _numThreads = 4;
#endif

		uint8_t const * map = nullptr;

		double n = 0;
		double horizontalFOV = 0;
		point3d location;
		const unsigned int width = 0, height = 0;
		const unsigned int percentBoundary = 0;
		vector<Cell> cells;

		matrix3d yRotate;
		matrix3d zRotate;
		Profile profile;

		motion3d motion;
	};
}

#endif