#ifndef CHECKERBOARD_HPP
#define CHECKERBOARD_HPP

#include "texture.hpp"

namespace tr {
	class CheckerBoard : public Texture_t {
	public:
		CheckerBoard(Light::rgb col, Light::rgb col2, double step) : colPrime(col), colSecond(col2), step(step) {

		}

		~CheckerBoard() {

		}

		virtual Light::rgb colour(const double x, const double y) {
			int a = (int) (std::abs(std::floor(x / step))) % 2;
			int b = (int) (std::abs(std::floor(y / step))) % 2;
			int check = (a + b) % 2;
			if (check > 0.5) {
				return colPrime;
			}
			return colSecond;
		}

		virtual Texture clone() {
			return std::make_unique<CheckerBoard>(colPrime, colSecond, step);
		}

	private:
		Light::rgb colPrime = Light::rgb(0);
		Light::rgb colSecond = Light::rgb(2.0);
		double step = 1;
	};
}

#endif