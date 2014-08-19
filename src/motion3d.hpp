//
//  motion3d.hpp
//  trayrace
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_motion3d_hpp
#define trayrace_motion3d_hpp

namespace tr {
    struct motion3d {
    public:
        motion3d() : translate(0), rotate() {}
        motion3d(const point3d& loc, const matrix3d& rot) : translate(loc), rotate(rot) {}
        motion3d(const motion3d& rhs) : translate(rhs.translate), rotate(rhs.rotate) {}
        ~motion3d() {}
        
        motion3d& operator=(const motion3d& rhs) {
            translate = rhs.translate;
            rotate = rhs.rotate;
            return *this;
        }
        
        point3d translate;
        matrix3d rotate;
    };
}
    
#endif
