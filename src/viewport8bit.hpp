#ifndef VIEWPORT8BIT_HPP
#define VIEWPORT8BIT_HPP

#include <stdint.h>
#include <vector>
#include <memory>

#include "light.hpp"
#include "viewport.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
	class Viewport8bit : public Viewport {
	public:
		struct rgb {
			rgb(uint8_t r=0, uint8_t g=0, uint8_t b=0) : r(r), g(g), b(b) {}
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};

		Viewport8bit(unsigned int width, unsigned int height)
		: width(width), height(height) {
			screen = std::make_shared<vector<rgb>>();

			for (unsigned int w = 0; w < width; ++w) {
				for (unsigned int h = 0; h < height; ++h) {
					screen->push_back(rgb());
				}
			}

		}

		~Viewport8bit() {

		}

		void put(rgb colour, unsigned int x, unsigned int y) {
			put(colour, y*width + x);
		}

		rgb get(unsigned int x, unsigned int y) {
			return get(y*width + x);
		}

		shared_ptr<vector<rgb>> ptr() {
			return screen;
		}

		void put(rgb colour, unsigned int index) {
			(*screen.get())[index] = colour;
		}

		void put(Light::rgb colour, unsigned int index) {
			//colour = colour / (colour + 1);
			colour = pow(colour, 0.4545);
			colour = clamp(colour, 0.0, 1.0);
			put(rgb((uint8_t) (colour.r * 255), (uint8_t) (colour.g * 255), (uint8_t) (colour.b * 255)), index);
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