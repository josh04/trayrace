//
//  triangle3d.hpp
//  trayrace
//
//  Created by Josh McNamee on 09/10/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_triangle3d_hpp
#define trayrace_triangle3d_hpp

#include "shape.hpp"
#include "plane3d.hpp"
#include "point3d.hpp"

namespace tr {
    class Triangle3d : public plane3d {
    public:
        Triangle3d(const point3d& a, const point3d& b, const point3d& c, Texture texture) : plane3d(a, b, c, std::move(texture)), a(a), b(b), c(c), lineA( (b-a)*coordTrans ), lineB( (c-b)*coordTrans ), lineC( (a-c)*coordTrans ) {
        
        }
        
        Triangle3d(const point3d& a, const point3d& b, const point3d& c, Light::rgb colour) : Triangle3d(a, b, c, std::make_unique<Paint>(colour)) {}
        
        ~Triangle3d() {
            
        }
        
        virtual bool intersection(const line3d& ray, double &distance, Light::rgb &colour) const {
            const double t = -((ray.point - location).dot(norm)) / (ray.direction.dot(norm));
            
            if (t < 0) {
                return false;
            }
            
            point3d spotA =  (a*coordTrans - ray.getPoint(t)*coordTrans);
            point3d spotB = (b*coordTrans - ray.getPoint(t)*coordTrans);
            point3d spotC = (c*coordTrans - ray.getPoint(t)*coordTrans);
            
            double check1 = (lineA.z * spotA.y - lineA.y * spotA.z);
            double check2 = (lineB.z * spotB.y - lineB.y * spotB.z);
            double check3 = (lineC.z * spotC.y - lineC.y * spotC.z);
            
            //check2 = 1.0;
            //check3 = 1.0;
            
            if (!(check1 >= 0 && check2 >= 0 && check3 >= 0)) {
                return false;
            }
            
            distance = t;
            colour = texture->colour(spotA.z, spotA.y);
            
            return true;
        }
        
        /*
        void move(const point3d& newA, const point3d& newB, const point3d& newC) {
            a = newA;
            b = newB;
            c = newC;
            refreshNormal();
        }
        
        virtual void move(const point3d& newLocation) {
            
        }*/
        
        point3d a, b, c;
    private:
        
        point3d lineA, lineB, lineC;
    };
}


#endif
