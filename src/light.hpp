#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <algorithm>
#include <memory>
#include "point3d.hpp"

using std::max;
using std::min;

namespace tr {
	class Shape;
	class Light {
	public:
		class rgb {
		public:
			rgb(double r, double g, double b)
				: r(r), g(g), b(b) {

			}

			rgb(double i = 0.0)
				: r(i), g(i), b(i) {

			}

			rgb(const rgb& copy)
				: r(copy.r), g(copy.g), b(copy.b) {

			}

			rgb& operator+=(const rgb& rhs) {
				r += rhs.r;
				g += rhs.g;
				b += rhs.b;
				return *this;
			}

			rgb& operator-=(const rgb& rhs) {
				r -= rhs.r;
				g -= rhs.g;
				b -= rhs.b;
				return *this;
			}

			void operator-() {
				r = -r;
				g = -g;
				b = -b;
			}

			~rgb() {

			}
			double r, g, b;
		private:
		};

		Light(point3d location, rgb intensity, std::shared_ptr<Shape> shape)
			: location(location), intensity(intensity) {
			this->shape = shape;
		}

		Light(const Light& rhs)
			: location(rhs.location), intensity(rhs.intensity), shape(rhs.shape) {

		}

		~Light() {

		}

		point3d location;
		rgb intensity;
		std::shared_ptr<Shape> shape;
	private:
	};

	Light::rgb operator+(Light::rgb lhs, const Light::rgb& rhs) {
		return (lhs += rhs);
	}

	Light::rgb operator-(Light::rgb lhs, const Light::rgb& rhs) {
		return (lhs -= rhs);
	}

	Light::rgb operator*(Light::rgb lhs, const Light::rgb& rhs) {
		return Light::rgb(lhs.r*rhs.r, lhs.g*rhs.g, lhs.b*rhs.b);
	}

	Light::rgb operator*(const Light::rgb& lhs, const double rhs) {
		return Light::rgb(lhs.r*rhs, lhs.g*rhs, lhs.b*rhs);
	}

	Light::rgb operator*(const double rhs, const Light::rgb& lhs) {
		return lhs*rhs;
	}

	Light::rgb operator/(const Light::rgb& lhs, const double rhs) {
		return lhs*(1 / rhs);
	}

	Light::rgb operator/(const Light::rgb& lhs, const Light::rgb& rhs) {
		return Light::rgb(lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b);
	}

	Light::rgb clamp(Light::rgb light, const double in, const double ax) {
		light.r = min(light.r, ax);
		light.g = min(light.g, ax);
		light.b = min(light.b, ax);

		light.r = max(light.r, in);
		light.g = max(light.g, in);
		light.b = max(light.b, in);
		return light;
	}


}

tr::Light::rgb pow(tr::Light::rgb light, double exp) {
	return tr::Light::rgb(std::pow(light.r, exp), std::pow(light.g, exp), std::pow(light.b, exp));
}

namespace std {
	tr::Light::rgb abs(const tr::Light::rgb& light) {
		return tr::Light::rgb(std::abs(light.r), std::abs(light.g), std::abs(light.b));
	}
}

#endif
