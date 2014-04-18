#ifndef SQUARE3D_HPP
#define SQUARE3D_HPP

#include "plane3d.hpp"

namespace tr {
	class Square3d : public plane3d {
	public:
		Square3d(double length, double distance, double theta, double phi, Texture texture)
			: plane3d(distance, theta, phi, std::move(texture)), length(length) {

		}


		Square3d(double length, double distance, double theta, double phi, Light::rgb colour)
			: Square3d(length, distance, theta, phi, std::make_unique<Paint>(colour)) {}

		~Square3d() {

		}


		virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {
			double t = -((ray.point - location).dot(norm)) / (ray.direction.dot(norm));
			if (t < 0) {
				return false;
			}

			distance = t;

			const point3d spot = ray.getPoint(t)*coordTrans;

			if ((std::abs(spot.x) > length / 2) || (std::abs(spot.y) > length / 2)) {
				return false;
			}
			
			colour = texture->colour(spot.x, spot.y);

			return true;
		}

	private:
		double length;
	};
}

#endif