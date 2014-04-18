#ifndef PAINT_HPP
#define PAINT_HPP

#include "texture.hpp"

namespace tr {
	class Paint : public Texture_t {
	public:
		Paint(Light::rgb colour) : col(colour) {

		}

		~Paint() {

		}

		virtual Light::rgb colour(const double x, const double y) {
			return col;
		}

		virtual Texture clone() {
			return std::make_unique<Paint>(col);
		}

	private:
		Light::rgb col = Light::rgb(0);
	};
}

#endif