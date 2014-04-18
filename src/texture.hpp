#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <memory>
#include "light.hpp"

namespace tr {
	class Texture_t;
	typedef std::unique_ptr<Texture_t> Texture;
	class Texture_t {
	public:
		Texture_t() {

		}

		~Texture_t() {

		}

		virtual Light::rgb colour(const double x, const double y) = 0;
		virtual Texture clone() = 0;

	private:
	};
}

#endif