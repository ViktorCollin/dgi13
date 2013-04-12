// Introduction lab that covers:
// * C++
// * SDL
// * 2D graphics
// * Plotting pixels
// * Video memory
// * Color representation
// * Linear interpolation
// * glm::vec3 and std::vector

#include <SDL.h>
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
SDL_Surface* screen;

// --------------------------------------------------------
// FUNCTION DECLARATIONS

void Draw();
void Interpolate(vec3 a, vec3 b, vector<vec3>& res);

// --------------------------------------------------------
// FUNCTION DEFINITIONS

int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	while( NoQuitMessageSDL() )
	{
		Draw();
	}
	vector<vec3> res(4);
	vec3 a(1,4,9.2);
	vec3 b(4,1,9.8);
	Interpolate(a,b,res);
	for(int i = 0; i < res.size(); ++i){
		cout << "( " 
		<< res[i].x << ", " 
		<< res[i].y << ", "
		<< res[i].z << " ) ";
	}
	cout << endl;
	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Draw(){

	//Define colours of the corners..
	vec3 topLeft(1,0,0); //red
	vec3 topRight(0,0,1); //blue
	vec3 bottomLeft(0,1,0); //green
	vec3 bottomRight(1,1,0); //yellow
	
	vector<vec3> leftSide(SCREEN_HEIGHT);
	vector<vec3> rightSide(SCREEN_HEIGHT);
	Interpolate(topLeft, bottomLeft, leftSide);
	Interpolate(topRight, bottomRight, rightSide);
	vector<vec3> colors(SCREEN_WIDTH);
	for(int y=0; y<SCREEN_HEIGHT; ++y ){
		Interpolate(leftSide[y], rightSide[y], colors);
		for( int x=0; x<SCREEN_WIDTH; ++x ){
			PutPixelSDL( screen, x, y, colors[x] );
		}
	}
	
	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
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
