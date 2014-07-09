#ifndef SQUARE3D_HPP
#define SQUARE3D_HPP

#include "plane3d.hpp"

namespace tr {
	class Square3d : public plane3d {
	public:
		Square3d(double length, double distance, double theta, double phi, double rho, Texture texture)
			: plane3d(distance, theta, phi, std::move(texture)), length(length), xrot(rho), rho(rho) {

		}


		Square3d(double length, double distance, double theta, double phi, double rho, Light::rgb colour)
			: Square3d(length, distance, theta, phi, rho, std::make_unique<Paint>(colour)) {}

		~Square3d() {

		}


		virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {
			double t = -((ray.point - location).dot(norm)) / (ray.direction.dot(norm));
			if (t < 0) {
				return false;
			}

			distance = t;

			const point3d spot = ray.getPoint(t)*coordTrans*xrot;

			if ((std::abs(spot.z) > length / 2) || (std::abs(spot.y) > length / 2)) {
				return false;
			}
			
			colour = texture->colour(spot.z, spot.y);

			return true;
		}

	private:
		double length;
        double rho;
        xRotation xrot;
	};
}

#endif