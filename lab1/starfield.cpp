// Introduction lab that covers:
// * C++
// * SDL
// * 2D graphics
// * Plotting pixels
// * Video memory
// * Color representation
// * Linear interpolation
// * glm::vec3 and std::vector

#include "SDL.h"
#include <iostream>
#include <glm/glm.hpp>
#include <vector>
#include "SDLauxiliary.h"

using namespace std;
using glm::vec3;

// --------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int NUM_STARS = 1000;
const float SPEED = 0.0002;
const float F = 75;
SDL_Surface* screen;
vector<vec3> stars(NUM_STARS);
int t;

// --------------------------------------------------------
// FUNCTION DECLARATIONS

void Draw();
void Interpolate(vec3 a, vec3 b, vector<vec3>& res);
void Update();

// --------------------------------------------------------
// FUNCTION DEFINITIONS

int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	vec3 r;
	for(int i = 0; i < NUM_STARS; ++i){
		r.x = (float(rand()) / float(RAND_MAX/2) -1);//*SCREEN_WIDTH/2+SCREEN_WIDTH/2;
		r.y = (float(rand()) / float(RAND_MAX/2) -1);//*SCREEN_HEIGHT/2;+SCREEN_HEIGHT/2;
		r.z = float(rand()) / float(RAND_MAX);
		stars[i] = r;
	}
	/*
	for(int i = 0; i < 10; ++i){
		cout << "( " 
		<< stars[i].x << ", " 
		<< stars[i].y << ", "
		<< stars[i].z << " ) ";
	}
	*/
	t = SDL_GetTicks();
	while( NoQuitMessageSDL() )
	{
		Update();
		Draw();
	}
	vector<vec3> res(4);
	vec3 a(1,4,9.2);
	vec3 b(4,1,9.8);
	Interpolate(a,b,res);
	cout << endl;
	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Draw(){
	vec3 white(1,1,1);
	SDL_FillRect(screen,0,0);
	if(SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	float u,v;
	for(size_t s = 0; s < NUM_STARS; ++s){
		float c = 0.2/(stars[s].z*stars[s].z);
		vec3 color(c,c,c);
		u = F * (stars[s].x/stars[s].z) + (SCREEN_WIDTH/2);
		v = F * (stars[s].y/stars[s].z) + (SCREEN_HEIGHT/2);
		PutPixelSDL(screen, u, v, color);
	}
	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);
	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

void Update(){
	int t2 = SDL_GetTicks();
	float dt = float(t2-t); //ms since last update
	t = t2;
	for(int i = 0; i < NUM_STARS; ++i){
		stars[i].z -= dt*SPEED;
		if(stars[i].z <= 0){
			stars[i].z += 1;
		}
		if(stars[i].z > 1){
			stars[i].z -= 1;
		}
	}
}

void Interpolate(vec3 a, vec3 b, vector<vec3>& result){
	float size = result.size()-1;
	float x_step = (b.x-a.x)/size;
	float y_step = (b.y-a.y)/size;
	float z_step = (b.z-a.z)/size;
	for(int i = 0; i < size+1; ++i){
		result[i].x = a.x+(i*x_step);
		result[i].y = a.y+(i*y_step);
		result[i].z = a.z+(i*z_step);
	}
}
