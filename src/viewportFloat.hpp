#ifndef VIEWPORTFLOAT_HPP
#define VIEWPORTFLOAT_HPP

#include <stdint.h>
#include <vector>
#include <memory>

#include "viewport.hpp"
#include "light.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
	class ViewportFloat : public Viewport {
	public:
		struct rgb {
			rgb(float r = 0, float g = 0, float b = 0) : r(r), g(g), b(b), a(1.0) {}
			float r;
			float g;
			float b;
			float a;
		};

		ViewportFloat(unsigned int width, unsigned int height)
			: width(width), height(height) {
			screen = std::make_shared<vector<rgb>>();

			for (unsigned int w = 0; w < width; ++w) {
				for (unsigned int h = 0; h < height; ++h) {
					screen->push_back(rgb());
				}
			}

		}

		~ViewportFloat() {

		}

		void put(rgb colour, unsigned int x, unsigned int y) {
			put(colour, y*width + x);
		}

		rgb get(unsigned int x, unsigned int y) {
			return get(y*width + x);
		}

		unsigned char * ptr() {
			return (unsigned char *)&(*screen)[0];
		}

		void put(rgb colour, unsigned int index) {
			(*screen.get())[index] = colour;
		}

		void put(Light::rgb colour, unsigned int index) {
			//colour = colour / (colour + 1);
			//colour = pow(colour, 0.4545);
			//colour = clamp(colour, 0.0, 1.0);
			put(rgb((float) (colour.r), (float) (colour.g), (float) (colour.b)), index);
		}

	private:
		unsigned int width, height;
		rgb get(unsigned int index) {
			return (*screen.get())[index];
		}

		shared_ptr<vector<rgb>> screen;
	};
}

#endif