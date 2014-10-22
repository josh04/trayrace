#ifndef SCENE_HPP
#define SCENE_HPP

#include <vector>
#include <memory>

#include "types.hpp"

using std::shared_ptr;
using std::vector;

#include "SceneStruct.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "viewport8bit.hpp"
#include "boundBox.hpp"
#include "boundOblong.hpp"
#include "plane3d.hpp"
#include "circle3d.hpp"
#include "square3d.hpp"
#include "cube.hpp"
#include "triangle3d.hpp"

#include "paint.hpp"
#include "checkerboard.hpp"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

using std::vector;
using std::shared_ptr;
using std::make_shared;

namespace tr {
	class Scene {
	public:
		Scene() {

		}

		void init(SceneStruct * config) {
			shapes = std::make_shared<vector<shared_ptr<Shape>>>();
			camera = std::make_shared<Camera>(config->CameraLocation, config->waistRotation, config->headTilt, config->horizontalFov, config->width, config->height);
			lights = std::make_shared<vector<shared_ptr<Light>>>();

			shared_ptr<Shape> redBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -3.5, 1), 2, std::make_unique<Paint>(Light::rgb(0.5, 0.0, 0.0))));
			redBall->reflective = 0.6;
			shapes->push_back(redBall);
			shared_ptr<Shape> yellowBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -3.5, -11), 2, std::make_unique<Paint>(Light::rgb(0.5, 0.5, 0.0))));
			yellowBall->reflective = 0.6;
			shapes->push_back(yellowBall);
			shared_ptr<Shape> greenBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, -6.5, -6), 2, std::make_unique<Paint>(Light::rgb(0.0, 0.5, 0.0))));
			greenBall->reflective = 0.6;
			shapes->push_back(greenBall);

			shared_ptr<BoundOblong> grayBox = make_shared<BoundOblong>(point3d(0, 0, -10), 20, 60, std::make_unique<Paint>(Light::rgb(0.5)));
			grayBox->reflective = 0.001;
			grayBox->setDownTexture(std::make_unique<CheckerBoard>(Light::rgb(0.5), Light::rgb(2.0), 6));
			grayBox->setLeftTexture(std::make_unique<CheckerBoard>(Light::rgb(0.5, 0.0, 0.0), Light::rgb(2.0), 6));
			grayBox->setRightTexture(std::make_unique<CheckerBoard>(Light::rgb(0.0, 0.5, 0.0), Light::rgb(2.0), 6));
			shapes->push_back(grayBox);

			shared_ptr<Shape> mirrorBall = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(1, 15, -6), 5, std::make_unique<Paint>(Light::rgb(0.75, 0.75, 0.75))));
			mirrorBall->reflective = 1.0;
			shapes->push_back(mirrorBall);

			shared_ptr<Shape> plane = std::static_pointer_cast<Shape>(make_shared<Square3d>(5, 5, 0, -60, 0, std::make_unique<CheckerBoard>(Light::rgb(0, 0.75, 0.75), Light::rgb(2.0, 2.0, 2.0), 1)));
			plane->reflective = 1.0;
			shapes->push_back(plane);
            
            triangles(shapes);
            
            //shared_ptr<Shape>
            cube = std::static_pointer_cast<Cube>(make_shared<Cube>(point3d(7, 3.5, 14), std::make_unique<CheckerBoard>(Light::rgb(0, 0.75, 0.75), Light::rgb(2.0, 2.0, 2.0), 1), 5, 0, 0));
			cube->reflective = 0.5;
			shapes->push_back(cube);
            
			shared_ptr<Shape> lampBallOne = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(6.0, -6.0, 9.0), 0.6, std::make_unique<Paint>(Light::rgb(0.9))));
			lampBallOne->reflective = 0.0;
			shared_ptr<Shape> lampBallTwo = std::static_pointer_cast<Shape>(make_shared<Sphere>(point3d(-9.0, 0.0, 0.0), 0.6, std::make_unique<Paint>(Light::rgb(0.9))));
			lampBallTwo->reflective = 0.0;

			shapes->push_back(lampBallOne);
            shapes->push_back(lampBallTwo);

			shared_ptr<Light> light = make_shared<Light>(point3d(6.0, -6.0, 9.0), Light::rgb(0.4), lampBallOne);
			shared_ptr<Light> light2 = make_shared<Light>(point3d(-9.0, 0.0, 0.0), Light::rgb(0.4), lampBallTwo);
			lights->push_back(light);
			lights->push_back(light2);
            /*
            const aiScene * scene = importer.ReadFile( "meshes/teapot.obj",
                                                     aiProcess_CalcTangentSpace       |
                                                     aiProcess_Triangulate            |
                                                     aiProcess_JoinIdenticalVertices  |
                                                     aiProcess_SortByPType);
            
            
            if (!scene) {
                return;
            }
            
            const aiMesh * mesh = scene->mMeshes[0];
            
            // Initialize the meshes in the scene one by one
            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                unsigned int ai = mesh->mFaces[i].mIndices[0];
                unsigned int bi = mesh->mFaces[i].mIndices[1];
                unsigned int ci = mesh->mFaces[i].mIndices[2];
                
                const point3d a(mesh->mVertices[ai].x, mesh->mVertices[ai].y, mesh->mVertices[ai].z);
                const point3d b(mesh->mVertices[bi].x, mesh->mVertices[bi].y, mesh->mVertices[bi].z);
                const point3d c(mesh->mVertices[ci].x, mesh->mVertices[ci].y, mesh->mVertices[ci].z);
                
                shared_ptr<Shape> triangle = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(a, b, c, std::make_unique<Paint>(Light::rgb(0.75, 0.75, 0.75))));
                
                triangle->reflective = 0.9;
                
                //shapes->push_back(triangle);
            }*/
            
		}

		void moveCamera(point3d move, int waistRotation, int headTilt, double horizontalFov) {
			camera->move(move, waistRotation, headTilt, horizontalFov);
		}
        
        void moveObject() {
            //cuberot = cuberot+1;
            point3d newcubeloc = cubeloc*yRotation(cuberot*(M_PI/180));
            //newcubeloc += point3d(5,0,0);
            cube->move(newcubeloc-cubeloc, 0.0, 0.0, 10);
            //cube->move(point3d(0), 0.0, 0.0, 10);
            cubeloc = newcubeloc;
        }

		~Scene() {}

		void preSnap(shared_ptr<Viewport> depthMap, shared_ptr<Viewport> normalMap, shared_ptr<Viewport> colourMap) {
            camera->activateAll();
			camera->preliminary_snap(depthMap, normalMap, colourMap, shapes, lights);
		}
        
        void snap(uint8_t const * redraw, shared_ptr<Viewport> viewport) {
            if (redraw != nullptr) {
                camera->activateMap(redraw);
            }
            camera->final_snap(viewport, shapes, lights);
        }

	private:
        void triangles(Shapes shapes) {
            
            shared_ptr<Shape> triangle = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                 point3d(19,3.5,10),
                                                                                                 point3d(17,3.5,12),
                                                                                                 point3d(17,0,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle2 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(19,3.5,10),
                                                                                                  point3d(17,3.5,8),
                                                                                                  point3d(17,0,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle3 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(17,7,10),
                                                                                                  point3d(17,3.5,12),
                                                                                                  point3d(19,3.5,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle4 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(17,7,10),
                                                                                                  point3d(17,3.5,8),
                                                                                                  point3d(19,3.5,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            // back
            
            shared_ptr<Shape> triangle5 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                 point3d(15,3.5,10),
                                                                                                 point3d(17,3.5,12),
                                                                                                 point3d(17,0,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle6 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(15,3.5,10),
                                                                                                  point3d(17,3.5,8),
                                                                                                  point3d(17,0,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle7 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(17,7,10),
                                                                                                  point3d(17,3.5,12),
                                                                                                  point3d(15,3.5,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            
            shared_ptr<Shape> triangle8 = std::static_pointer_cast<Shape>(make_shared<Triangle3d>(
                                                                                                  point3d(17,7,10),
                                                                                                  point3d(17,3.5,8),
                                                                                                  point3d(15,3.5,10), std::make_unique<CheckerBoard>(Light::rgb(0.75, 0.0, 0.75), Light::rgb(2.0, 2.0, 0.0), 1)));
            double refl = 0.9;
            triangle->reflective = refl;
            triangle2->reflective = refl;
            triangle3->reflective = refl;
            triangle4->reflective = refl;
            triangle5->reflective = refl;
            triangle6->reflective = refl;
            triangle7->reflective = refl;
            triangle8->reflective = refl;
            
            shapes->push_back(triangle);
            shapes->push_back(triangle2);
            shapes->push_back(triangle3);
            shapes->push_back(triangle4);
            shapes->push_back(triangle5);
            shapes->push_back(triangle6);
            shapes->push_back(triangle7);
            shapes->push_back(triangle8);
        }
        
        double cuberot = 5.0;
        point3d cubeloc = point3d(1, -3.5, 1);
		Shapes shapes = nullptr;
		Lights lights = nullptr;
		shared_ptr<Camera> camera = nullptr;
        
        shared_ptr<Cube> cube = nullptr;
        
        Assimp::Importer importer;
	};

}

#endif