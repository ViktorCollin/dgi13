#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include <vector>
#include <limits>
#define USE_MATH_DEFINES
#include <cmath>

using namespace std;
using glm::vec3;
using glm::mat3;

// ----------------------------------------------------------------------------
// STRUCTS
struct Intersection{
	vec3 position;
	float distance;
	int triangleIndex;
};

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = SCREEN_WIDTH;
const float MOVING_SPEED = 0.1;
const float ROTATION_SPEED = M_PI/32;
SDL_Surface* screen;
vector<Triangle> triangles;
float focalLength = SCREEN_HEIGHT;
vec3 cameraPos(0.0, 0.0, -3.0);
float yaw = 0.0;
mat3 R;
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightColor = 14.f * vec3( 1, 1, 1 );
vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );
int t;

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection);
vec3 DirectLight( const Intersection& i );
void initR();
void updateR();

int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	LoadTestModel( triangles );
	initR();
	
	t = SDL_GetTicks();	// Set start value for timer.
	while( NoQuitMessageSDL() )
	{
		Update();
		Draw();
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;
	Uint8* keystate = SDL_GetKeyState( 0 );
	vec3 x(MOVING_SPEED, 0.0, 0.0);
	vec3 y(0.0, MOVING_SPEED, 0.0);
	vec3 z(0.0, 0.0, MOVING_SPEED);
	
	
	if( keystate[SDLK_UP] ){
		cameraPos += R*z;
	}
	if( keystate[SDLK_DOWN] ){
		cameraPos -= R*z;	
	}
	if( keystate[SDLK_LEFT] ){
		yaw += ROTATION_SPEED;
		updateR();
	}
	if( keystate[SDLK_RIGHT] ){
		yaw -= ROTATION_SPEED;
		updateR();
	}
	if( keystate[SDLK_w] ){
		lightPos += R*z;
	}
	if( keystate[SDLK_s] ){
		lightPos -= R*z;
	}
	
	if( keystate[SDLK_a] ){
		lightPos -= R*x;
	}
	if( keystate[SDLK_d] ){
		lightPos += R*x;
	}
}

void Draw()
{
	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	for( int y=0; y<SCREEN_HEIGHT; ++y ){
		for( int x=0; x<SCREEN_WIDTH; ++x ){
			vec3 dir(x-SCREEN_WIDTH/2, y-SCREEN_HEIGHT/2, focalLength);
			Intersection hit;
			if(ClosestIntersection(cameraPos, R*dir, triangles, hit)){
				//color = triangles[hit.triangleIndex].color;
				PutPixelSDL( screen, x, y, triangles[hit.triangleIndex].color*(indirectLight + DirectLight(hit)) );
			} else {
				PutPixelSDL( screen, x, y, vec3(0.0, 0.0, 0.0) );
			}
 
		}
	}

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

bool ClosestIntersection(vec3 start, vec3 dir, const vector<Triangle>& triangles, Intersection& closestIntersection){
	closestIntersection.distance = numeric_limits<float>::max();
	closestIntersection.triangleIndex = -1;
	for(int i=0; i<triangles.size(); ++i){
		vec3 e1 = triangles[i].v1 - triangles[i].v0;
		vec3 e2 = triangles[i].v2 - triangles[i].v0;
		vec3 b = start - triangles[i].v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = glm::inverse( A ) * b;
		if(0.00001 < x.x  && 0.0 <= x.y && 0.0 <= x.z && x.y+x.z <= 1 && x.x < closestIntersection.distance){
			closestIntersection.distance = x.x;
			closestIntersection.triangleIndex = i;
			closestIntersection.position = start + x.x*dir;
			
		}
	}
	return closestIntersection.triangleIndex != -1;
}

vec3 DirectLight(const Intersection& i){
	vec3 lightDir = lightPos-i.position;
	float s = glm::dot(lightDir, triangles[i.triangleIndex].normal);
	if(s < 0.00001) return vec3(0.0, 0.0, 0.0);
	float dist = glm::length(lightDir);
	lightDir = glm::normalize(lightDir);
	Intersection hit;
	if (ClosestIntersection(i.position, lightDir, triangles, hit) && glm::length(i.position-hit.position) < (dist-0.00001)){
			return vec3(0.0, 0.0, 0.0);
	}else{
		return (lightColor * s/(float(4*M_PI)*dist*dist));
	}
	
}

void initR(){
 	R[0][0] = cos(yaw);
 	R[0][1] = 0;
 	R[0][2] = sin(yaw);

 	R[1][0] = 0;
 	R[1][1] = 1;
 	R[1][2] = 0;

 	R[2][0] = -R[0][2];
 	R[2][1] = 0;
 	R[2][2] = R[0][0];
}
void updateR(){
	R[0][0] = cos(yaw);
	R[0][2] = sin(yaw);

	R[2][0] = -R[0][2];
	R[2][2] = R[0][0];
}