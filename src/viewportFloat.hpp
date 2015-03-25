#ifndef VIEWPORTFLOAT_HPP
#define VIEWPORTFLOAT_HPP

#include <stdint.h>
#include <vector>
#include <memory>

#include <Video Mush/opencl.hpp>
#include <Video Mush/imageProcess.hpp>

#include "viewport.hpp"
#include "light.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
    class ViewportFloat : public Viewport, public mush::imageProcess {
	public:
		struct rgb {
			rgb(float r = 0, float g = 0, float b = 0) : r(r), g(g), b(b), a(1.0) {}
			float r;
			float g;
			float b;
			float a;
		};

		ViewportFloat(unsigned int width, unsigned int height)
        : mush::imageProcess() {
            _width = width;
            _height = height;
			screen = std::make_shared<vector<rgb>>();

			for (unsigned int w = 0; w < _width; ++w) {
				for (unsigned int h = 0; h < _height; ++h) {
					screen->push_back(rgb());
				}
			}
            
		}

		~ViewportFloat() {

		}
        
        void init(std::shared_ptr<mush::opencl> context, std::vector<std::shared_ptr<mush::ringBuffer>> buffers) {
            queue = context->getQueue();
            
            im = context->floatImage(_width, _height);
            
            addItem(im);
        }
        
        void process() {
            auto ptr = inLock();
            if (ptr == nullptr) {
                return;
            }
            
            cl::Event event;
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            
            queue->enqueueWriteImage(*(cl::Image2D *)_getMem(0), CL_TRUE, origin, region, 0, 0, &(*screen)[0], NULL, &event);
            event.wait();
            
            inUnlock();
        }

		void put(rgb colour, unsigned int x, unsigned int y) {
			put(colour, y*_width + x);
		}

		rgb get(unsigned int x, unsigned int y) {
			return get(y*_width + x);
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
        
        cl::CommandQueue * queue = nullptr;
        cl::Image2D * im = nullptr;
        
		rgb get(unsigned int index) {
			return (*screen.get())[index];
		}

		shared_ptr<vector<rgb>> screen;
	};
}

#endif