#ifndef CELL_HPP
#define CELL_HPP

#define _EPSILON 1e-5
#define _REFLECTIONS 2

#include <vector>
#include <thread>
#include <typeinfo>
#include <memory>

#include "point3d.hpp"
#include "shape.hpp"
#include "line3d.hpp"
#include "light.hpp"

using std::vector;
using std::shared_ptr;

namespace tr {
	class Cell {
	public:
		Cell(const unsigned int& x, const unsigned int& y, const unsigned int& w, const unsigned int& h)
			: ray(line3d()), x(x), y(y), w(w), h(h), u(point3d(0)), v(point3d(0)) {

		}

		~Cell() {

		}
        
        void activate() {
            active = true;
        }
        
        void deactivate() {
            active = false;
        }
        
        bool isActive() const {
            return active;
        }
        
        void move(const line3d& ray, const point3d& u, const point3d& v) {
            this->ray = ray;
            this->u = u;
            this->v = v;
            isCached = false;
        }
        
        struct rayProps {
            Light::rgb colour = Light::rgb(1.0, 1.0, 1.0);
            point3d normal = point3d(1.0, 0, 0);
            double depth = 1e9;
            point3d pointOnShape = point3d(0,0,0);
            const Shape * intersectedShape;
            point3d movement;
            
            bool failedToHit = false;
        };
        
        struct returnProps {
            double depth = 0.0;
            point3d normal = point3d(1.0, 0 ,0);
            Light::rgb colourMap = Light::rgb(1.0);
            point3d movement = point3d(0);
        };
        
        static Light::rgb fire_ray(const Shapes& shapes, const Lights& lights, const line3d& ray, rayProps& props, const unsigned long recurse = 0) {
            fire_primary(shapes, ray, props);
            Light::rgb colour(0);
            colour += fire_lights(shapes, lights, ray, props);
            colour += fire_reflections(shapes, lights, ray, props, recurse);
            return colour;
        }
        
        const Light::rgb preliminary_fire(Shapes shapes, Lights lights, const motion3d& cameraMotion, returnProps &reProps) {
            rayProps props;
            fire_primary(shapes, ray, props);
            Light::rgb colour(0);
            if (props.failedToHit) {
                return colour;
            }
            const point3d pt = fire_reprojection(props, ray, cameraMotion, w, h);
            
            props.movement = pt;
            
            cachedProps = props;
            isCached = true;
            
            /*if (x % 100 == 0 && y % 100 == 0) {
             if (abs(x - pt.x) > 1) {
             std::cout << "x: " << x << " y: " << y << " new x: " << pt.x << " new y: " << pt.y << std::endl;
             }
             }*/
            
            reProps.depth = props.depth;
            reProps.normal = props.normal;
            reProps.colourMap = props.colour;
            
            reProps.movement.x = x - props.movement.x;
            reProps.movement.y = y - props.movement.y;
            reProps.movement.z = props.movement.z;
            
            return colour;
        }

		const Light::rgb final_fire(Shapes shapes, Lights lights, const motion3d& cameraMotion, returnProps &reProps) const {
            rayProps props;
            Light::rgb colour(0);
            if (isCached) {
                props = cachedProps;
            } else {
                fire_primary(shapes, ray, props);
                if (props.failedToHit) {
                    return colour;
                }
                const point3d pt = fire_reprojection(props, ray, cameraMotion, w, h);
                
                props.movement = pt;
            }
            
            /*if (x % 100 == 0 && y % 100 == 0) {
                if (abs(x - pt.x) > 1) {
                std::cout << "x: " << x << " y: " << y << " new x: " << pt.x << " new y: " << pt.y << std::endl;
                }
            }*/
        
            colour += fire_lights(shapes, lights, ray, props);
            colour += fire_reflections(shapes, lights, ray, props, 0);
            
            reProps.depth = props.depth;
            reProps.normal = props.normal;
            reProps.colourMap = props.colour;
            
            reProps.movement.x = x - props.movement.x;
            reProps.movement.y = y - props.movement.y;
            reProps.movement.z = props.movement.z;
            
            return colour;
		}
/*
		const Light::rgb fire4(Shapes shapes, Lights lights, const motion3d& cameraMotion, returnProps& props) const {
			const line3d ray1(ray.point, ray.direction + (-0.25 / h)*u + (-0.25 / w)*v);
			const line3d ray2(ray.point, ray.direction + (-0.25 / h)*u + (+0.25 / w)*v);
			const line3d ray3(ray.point, ray.direction + (+0.25 / h)*u + (-0.25 / w)*v);
			const line3d ray4(ray.point, ray.direction + (+0.25 / h)*u + (+0.25 / w)*v);
            
            rayProps props1, props2, props3, props4;
            
/*			Light::rgb colour1 = fire(std::ref(shapes), std::ref(lights), ray1, cameraMotion, props1);
			Light::rgb colour2 = fire(std::ref(shapes), std::ref(lights), ray2, cameraMotion, props2);
			Light::rgb colour3 = fire(std::ref(shapes), std::ref(lights), ray3, cameraMotion, props3);
			Light::rgb colour4 = fire(std::ref(shapes), std::ref(lights), ray4, cameraMotion, props4);*

			Light::rgb colour;
			colour.r = (colour1.r + colour2.r + colour3.r + colour4.r) / 4;
			colour.g = (colour1.g + colour2.g + colour3.g + colour4.g) / 4;
			colour.b = (colour1.b + colour2.b + colour3.b + colour4.b) / 4;

            props.depth = (props1.depth+props2.depth+props3.depth+props4.depth) / 4;
            props.normal = (props1.normal+props2.normal+props3.normal+props4.normal) / 4;
            props.colourMap = (props1.colour + props2.colour + props3.colour + props4.colour) / 4;
            
            props.movement.x = x - (props1.movement.x + props2.movement.x + props3.movement.x + props4.movement.x) / 4;
            props.movement.y = y - (props1.movement.y + props2.movement.y + props3.movement.y + props4.movement.y) / 4;
            props.movement.z = (props1.movement.z + props2.movement.z + props3.movement.z + props4.movement.z) / 4;
            
			return colour;
		}*/

	private:
        static const void fire_primary(const Shapes& shapes, const line3d& ray, rayProps& props) {
            double least_distance = 10000.0;
			Light::rgb colour(0.0);
			std::shared_ptr<const Shape> intersectedShape = nullptr;
			
			Light::rgb shapeColour(0);
			Light::rgb shapeTemp(0);
			double distance = 1e9;
            
            unsigned long shape_count = shapes->size();
			for (unsigned long i = 0; i < shape_count; ++i) {
				shared_ptr<Shape>& shape = std::ref((*shapes)[i]);
				if (shape->intersection(ray, distance, shapeTemp)) {
					if (distance < least_distance && distance > _EPSILON) {
						least_distance = distance;
						intersectedShape = shape;
						shapeColour = shapeTemp;
					}
				}
			}
            
            props.intersectedShape = intersectedShape.get();
            
			if (props.intersectedShape == nullptr) {
				props.failedToHit = true;
                props.colour = Light::rgb(1e9);
                return;
			}
            
			props.pointOnShape = ray.getPoint(least_distance);
			props.normal = intersectedShape->normal(props.pointOnShape);
            
            Light::rgb whiteMod = 0.001; //make everything bright2white
            
            props.colour = shapeColour + whiteMod;
            props.depth = least_distance;
        }
        
        static const point3d fire_reprojection(const rayProps& props, const line3d& ray, const motion3d& cameraMotion, const double width, const double height) {
            
            motion3d reloc = props.intersectedShape->getMotion();
            
            const point3d relocatedPointOnShape = ((props.pointOnShape+reloc.translate) - (props.intersectedShape->getLocation()+reloc.translate))*reloc.rotate + (props.intersectedShape->getLocation() + reloc.translate);
            
            const point3d dir(relocatedPointOnShape - (cameraMotion.translate));
              
            point3d transDir = dir*cameraMotion.rotate;
            
			double n = tan(90.0 / 2.0*(M_PI / 180.0));
            
            double div = transDir.x/n;
            
            transDir = transDir/div;
            
            const double ratio = height/width;
            
            transDir.y = transDir.y / ratio;
            
            double v = (((transDir.y + 1.0)/2.0) * height)-0.5;
            double u = (((transDir.z + 1.0)/2.0) * width)-0.5;
            
            return point3d(u, v, dir.magnitude());
        }
        
        static const Light::rgb fire_light(const Shapes& shapes, const Light& light, const line3d& ray, const rayProps& props) {
            point3d di = light.location - props.pointOnShape;
            double light_distance = di.magnitude();
            line3d lray(props.pointOnShape, di.unit());
            bool blocked = false;
            
            double id = 1e9;
            Light::rgb temp(0);
            
            unsigned long shape_count = shapes->size();
            for (int i = 0; i < shape_count; ++i) {
                shared_ptr<Shape>& lshape = std::ref((*shapes)[i]);
                if (lshape.get() != light.shape.get()) {
                    if (lshape->intersection(lray, id, temp)) {
                        if (id < light_distance && id > _EPSILON) {
                            blocked = true;
                        }
                    }
                }
                if (blocked) {
                    break;
                }
            }
            
            Light::rgb colour(0);
            
            if (props.intersectedShape == nullptr) {
                return Light::rgb(100);
            }
            
            if (!blocked && (props.intersectedShape != light.shape.get())) {
                // diffuse lighting
                
                double nm = lray.direction.dot(props.normal);
                if (nm < 0) {
                    nm = 0;
                }
                colour += (1.0 - (props.intersectedShape->reflective)) * ((props.colour)*((light.intensity*nm)));
                
                // phong highlights
                unit3d& ldi = lray.direction;
                unit3d rdi = -(ray.direction);
                unit3d h = (ldi + rdi).unit();
                
                Light::rgb highlight = Light::rgb(0);
                
                highlight = light.intensity*(pow(h.dot(props.normal), 128.0));
                
                colour += (1.0 - (props.intersectedShape->reflective)) * highlight;
                
            }
            
            return colour;
        }
        
        static const Light::rgb fire_lights(const Shapes& shapes, const Lights& lights, const line3d& ray, const rayProps& props) {
            
			Light::rgb colour = Light::rgb(0);
            
            Light::rgb amb = 0.01; //ambient term
            
            colour += ((props.colour)*(amb));
            
			unsigned long lights_count = lights->size();
			for (unsigned long j = 0; j < lights_count; ++j) {
                colour += fire_light(shapes, *(*lights)[j], ray, props);
			}
            return colour;
        }
        
        static const Light::rgb fire_reflections(const Shapes& shapes, const Lights& lights, const line3d& ray, const rayProps& props, const unsigned long recurse = 0) {
			// specular reflections
			// find direction
            
			Light::rgb reflection = Light::rgb(0);
            
            if (props.intersectedShape == nullptr) {
                return reflection;
            }
			if (props.intersectedShape->reflective > 0) {
				if (recurse < _REFLECTIONS) {// && intersectedShape->specular) {
					point3d pt = ray.direction - 2 * (ray.direction.dot(props.normal)) * props.normal;
                    rayProps reflectProps;
					reflection = Cell::fire_ray(shapes, lights, line3d(props.pointOnShape, pt), reflectProps, recurse + 1);
					reflection = props.intersectedShape->reflective*reflection + (1.0 - props.intersectedShape->reflective)*reflection*props.colour;
				}
			}
            
            return props.intersectedShape->reflective * reflection;
        }
        
        
		line3d ray;
		const unsigned int x, y, w, h;
		point3d u, v;
        
        bool isCached = false;
        rayProps cachedProps;
        
        bool active = true;
	};
}

#endif