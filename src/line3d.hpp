#ifndef LINE3D_HPP
#define LINE3D_HPP

#include "point3d.hpp"

namespace tr {
	class line3d {
	public:
		line3d(point3d point, unit3d direction) 
		: point(point), direction(direction) {
		}

		line3d(const line3d& line)
			: point(line.point), direction(line.direction) {
		}

		~line3d() {

		}

		line3d& operator=(line3d& rhs) {
			point = rhs.point;
			direction = rhs.direction;
			return *this;
		}

		point3d intersect() {
			
		}

		const point3d getPoint(double distance) const {
			return point3d(point + distance*direction);
		}

		point3d point;
		unit3d direction;
	private:
		
	};
}

#endif