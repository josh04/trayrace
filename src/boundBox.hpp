#ifndef BOUNDBOX_HPP
#define BOUNDBOX_HPP

#include "shape.hpp"
#include "boundOblong.hpp"

namespace tr {
	class BoundBox : public BoundOblong {
	public:
		BoundBox(point3d location, double dimension, Texture texture)
			: BoundOblong(location, dimension, dimension, std::move(texture)) {

		}

		~BoundBox() {

		}

	private:
	};
}

#endif