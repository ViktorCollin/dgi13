#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#define USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <algorithm>
#include <cassert>

using namespace std;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES
const bool DEBUG = false;
const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = SCREEN_WIDTH;
const float MOVING_SPEED = 0.001;
const float ROTATION_SPEED = 0.001;
SDL_Surface* screen;
vector<Triangle> triangles;
float focalLength = SCREEN_HEIGHT;
vec3 cameraPos(0.0, 0.0, -3.0);
float yaw = 0.0;
mat3 R;
vec3 currentColor;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

int t;

struct Pixel{
	int x;
	int y;
	float zinv;
	friend ostream& operator<< (ostream& os, const Pixel & p);
};

ostream& operator<< (ostream& os, const Pixel & p){
	os << "(" << p.x <<", "<<p.y<<")";
	return os;
}

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
void initR();
void updateR();
void VertexShader(const vec3&, Pixel &);
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void DrawLineSDL( SDL_Surface* surface, Pixel a, Pixel b, vec3 color );
void DrawPolygonEdges(const vector<vec3> & vertices);
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels );
void DrawRows( const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels );
void DrawPolygon( const vector<vec3>& vertices );


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

	Uint8* keystate = SDL_GetKeyState(0);
	vec3 x(MOVING_SPEED*dt, 0.0, 0.0);
	vec3 y(0.0, MOVING_SPEED*dt, 0.0);
	vec3 z(0.0, 0.0, MOVING_SPEED*dt);

	if( keystate[SDLK_UP] ){
		cameraPos += R*z;
	}
	if( keystate[SDLK_DOWN] ){
		cameraPos -= R*z;	
	}
	if( keystate[SDLK_LEFT] ){
		yaw += ROTATION_SPEED*dt;
		updateR();
	}
	if( keystate[SDLK_RIGHT] ){
		yaw -= ROTATION_SPEED*dt;
		updateR();
	}
	if( keystate[SDLK_RSHIFT] )
		;

	if( keystate[SDLK_RCTRL] )
		;

	if( keystate[SDLK_w] )
		;

	if( keystate[SDLK_s] )
		;

	if( keystate[SDLK_d] )
		;

	if( keystate[SDLK_a] )
		;

	if( keystate[SDLK_e] )
		;

	if( keystate[SDLK_q] )
		;
}

void Draw()
{
	for( int y=0; y<SCREEN_HEIGHT; ++y ){
		for( int x=0; x<SCREEN_WIDTH; ++x ){
			depthBuffer[y][x] = 0;
		}
	}
	SDL_FillRect( screen, 0, 0 );

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);
	
	for( int i=0; i<triangles.size(); ++i )
	{
		vector<vec3> vertices(3);

		vertices[0] = triangles[i].v0;
		vertices[1] = triangles[i].v1;
		vertices[2] = triangles[i].v2;
		currentColor = triangles[i].color;
		if (DEBUG) cerr << "Start drawing triangle: " << i << endl;
		DrawPolygon(vertices);
	}
	
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

void VertexShader(const vec3 & v, Pixel & p){
	if(DEBUG) cerr << "VertexShader: (" << v.x << ", " << v.y << ", " << v.z << ")" << endl;
	vec3 prim = (v-cameraPos)*R;
	p.x = focalLength * prim.x/prim.z + SCREEN_WIDTH/2;
	p.y = focalLength * prim.y/prim.z + SCREEN_HEIGHT/2;
	p.zinv = 1/prim.z;
}

void Interpolate( Pixel a, Pixel b, vector<Pixel>& result ){
	float size = result.size()-1;
	if(DEBUG) cerr << "Interpolate: (" << a.x << ", " << a.y << ") to (" << b.x << ", " << b.y << ") in " << size+1 << "steps" << endl;
	float stepX = (b.x-a.x) / max(size, 1.f);
	float stepY = (b.y-a.y) / max(1.f, size);
	float stepZinv = (b.zinv-a.zinv) / max(size, 1.f);
	Pixel current = a;
	for (size_t i = 0; i < size+1; i++){
		result[i].x = int(current.x+i*stepX);
		result[i].y = int(current.y+i*stepY);
		result[i].zinv = double(current.zinv+i*stepZinv);
/*		
		result[i] = current;
		current.x += stepX;
		current.y += stepY;
		current.zinv += stepZinv;
*/		
	}
}

void DrawHorisontalLine(SDL_Surface* surface, Pixel a, Pixel b, vec3 color){
	assert(a.y == b.y);
	if(DEBUG) cerr << "DrawHorisontalLine: (" << a.x << ", " << a.y << ") to (" << b.x << ", " << b.y << ") in color("
	 	<< color.x << ", " << color.y << ", " << color.z << ")" << endl;
	if(a.x > b.x) swap(a,b);
	for(int i=a.x; i<=b.x; ++i){
		PutPixelSDL(surface, i, a.y, color);
	}
}

void DrawLineSDL( SDL_Surface* surface, Pixel a, Pixel b, vec3 color ){
	if(DEBUG) cerr << "DrawLineSDL: (" << a.x << ", " << a.y << ") to (" << b.x << ", " << b.y << ") in color("
	 	<< color.x << ", " << color.y << ", " << color.z << ")" << endl;
	
	int dx = std::abs( a.x - b.x );
	int dy = std::abs( a.y - b.y );
	int pixels = max( dx, dy ) + 1;
	vector<Pixel> line( pixels );
	Interpolate( a, b, line );
	for (vector<Pixel>::iterator it = line.begin(); it != line.end(); it++){
		if(it->zinv > depthBuffer[it->x][it->y]){
			PutPixelSDL(surface, it->x, it->y, color);
		}
	}

}

void DrawPolygonEdges(const vector<vec3> & vertices){
	size_t V = vertices.size();
	vector<Pixel> projected(V);
	for (size_t i = 0; i < V; i++){
		VertexShader(vertices[i], projected[i]);
	}

	for (size_t i = 0; i < V; i++){
		int j = (i+1)%V;
		vec3 color(1, 1, 1);
		DrawLineSDL(screen, projected[i], projected[j], color);
	}
}

void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels ){
	if(DEBUG) cerr << "ComputePolygonRows: " << endl;
	
	int min_y = numeric_limits<int>::max();
	int max_y = numeric_limits<int>::min();
	
	for(int i=0; i<vertexPixels.size(); ++i){
		if(vertexPixels[i].y < min_y) min_y = vertexPixels[i].y;
		if(vertexPixels[i].y > max_y) max_y = vertexPixels[i].y;
	}

	int lenght = (max_y - min_y) + 1;
	leftPixels.resize(lenght);
	rightPixels.resize(lenght);
	for(int i=0; i<lenght; ++i){
		leftPixels[i].x = numeric_limits<int>::max();
		leftPixels[i].y = min_y + i;
		rightPixels[i].x = numeric_limits<int>::min();
		rightPixels[i].y = min_y + i;
	}
	
	for(size_t i=0; i<vertexPixels.size(); ++i){
		size_t j = (i+1) % vertexPixels.size();
		/*
		int dx = std::abs(vertexPixels[i].x - vertexPixels[j].x);
		int dy = std::abs(vertexPixels[i].y - vertexPixels[j].y);
		int pixels = max( dx, dy ) + 1;
		*/
		ivec2 d = glm::abs(ivec2(vertexPixels[i].x,vertexPixels[i].y) - ivec2(vertexPixels[j].x, vertexPixels[j].y));
		int pixels = glm::max(d.x,d.y) + 1;
		vector<Pixel> line( pixels );
		Interpolate(vertexPixels[i], vertexPixels[j], line );
		for (vector<Pixel>::const_iterator it = line.begin(); it != line.end(); it++){
			if (it->y < min_y || it->y > max_y) continue;
			if(rightPixels[it->y - min_y].x < (it->x)){
				rightPixels[it->y - min_y] = *it;
			}
			if(leftPixels[it->y - min_y].x > (it->x)){
				leftPixels[it->y - min_y] = *it;
			}
		}
	}
}

void PixelShader(const Pixel & p){
	if (p.x < 0 || p.x >= SCREEN_WIDTH || p.y < 0 || p.y >= SCREEN_HEIGHT) return;
	if(p.zinv + 0.0001 > depthBuffer[p.x][p.y]){
		PutPixelSDL(screen, p.x, p.y, currentColor);
		depthBuffer[p.x][p.y] = p.zinv;
	}
}

void DrawPolygonRows( const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels ){
	if(DEBUG) cerr << "DrawPolygonRows: " << leftPixels.size() << " to draw" << endl;
	
	for(int i=0; i<leftPixels.size(); ++i){
		ivec2 delta = glm::abs(ivec2(leftPixels[i].x, leftPixels[i].y) - ivec2(rightPixels[i].x, rightPixels[i].y));
		int pixels = glm::max(delta.x, delta.y) + 1;
		vector<Pixel> line(pixels);
		Interpolate(leftPixels[i], rightPixels[i], line);

		for(vector<Pixel>::const_iterator it = line.begin(); it != line.end(); it++){
	//		cerr << *it << endl;
			PixelShader(*it);
		}
	}
	
}

void DrawPolygon( const vector<vec3>& vertices ){
	int V = vertices.size();
	vector<Pixel> vertexPixels( V );
	for( int i=0; i<V; ++i )
		VertexShader( vertices[i], vertexPixels[i] );
	vector<Pixel> leftPixels;
	vector<Pixel> rightPixels;
//
	ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
//
	DrawPolygonRows( leftPixels, rightPixels );
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
