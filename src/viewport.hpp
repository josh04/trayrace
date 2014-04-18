#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include <stdint.h>
#include <vector>
#include <memory>

#include "light.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
	class Viewport {
	public:
		Viewport() {

		}

		~Viewport() {

		}

		virtual void put(Light::rgb colour, unsigned int index) = 0;

	protected:
	};
}

#endif