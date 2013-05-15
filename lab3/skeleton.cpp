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
const bool USE_SHADOWS = false;
const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = SCREEN_WIDTH;
const float MOVING_SPEED = 0.001;
const float ROTATION_SPEED = 0.001;
SDL_Surface* screen;
vector<Triangle> triangles;
float focalLength = SCREEN_HEIGHT;
vec3 cameraPos(0.0, 0.0, -3.0);
vec3 lightPos(0,-0.5,-0.7);
vec3 lightPower = 14.f*vec3( 1, 1, 1 );
vec3 indirectLightPower = 0.5f*vec3( 1, 1, 1 );
float yaw = 0.0;
mat3 R;
vec3 currentColor;
vec3 currentNormal;
vec3 currentReflectance;
double depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

int t;

struct Pixel{
	int x;
	int y;
	double zinv;
	vec3 illumination;
	vec3 pos3d;
	friend ostream& operator<< (ostream& os, const Pixel & p);
};

ostream& operator<< (ostream& os, const Pixel & p){
	os << "(" << p.x <<", "<<p.y<<")";
	return os;
}

struct Vertex{
	vec3 position;
	vec3 normal;
	vec3 reflectance;
	Vertex(vec3 & pos, vec3 & norm, vec3 & ref): position(pos), normal(norm), reflectance(ref){}
	Vertex(){}
};

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
void initR();
void updateR();
void VertexShader(const Vertex & v, Pixel & p);
void PixelShader(Pixel & p);
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void DrawLineSDL( SDL_Surface* surface, Pixel a, Pixel b, vec3 color );
void DrawPolygonEdges(const vector<vec3> & vertices);
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels );
void DrawRows( const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels );
void DrawPolygon( const vector<Vertex>& vertices );
void DrawDepthBuffer();
float Shadow(vec3 start, vec3 dir);


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
	if( keystate[SDLK_e] )
		;

	if( keystate[SDLK_q] )
		;
}

void Draw()
{
	for( int y=0; y<SCREEN_HEIGHT; ++y ){
		for( int x=0; x<SCREEN_WIDTH; ++x ){
			depthBuffer[y][x] = 0.0;
		}
	}
	SDL_FillRect( screen, 0, 0 );

	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);
	
	for( int i=0; i<triangles.size(); ++i )
	{
		vector<Vertex> vertices(3);
		vertices[0] = Vertex(triangles[i].v0, triangles[i].normal, triangles[i].color);
		vertices[1] = Vertex(triangles[i].v1, triangles[i].normal, triangles[i].color);
		vertices[2] = Vertex(triangles[i].v2, triangles[i].normal, triangles[i].color);
		
		currentNormal = triangles[i].normal;
		currentReflectance = currentColor = triangles[i].color;
		if (DEBUG) cerr << "Start drawing triangle: " << i << endl;
		DrawPolygon(vertices);
		//DrawDepthBuffer();
	}
	
	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}
vec3 DirectLight(const Vertex& i){
	vec3 lightDir = lightPos-i.position;
	float dist = glm::length(lightDir);
	float s = glm::dot(i.normal, glm::normalize(lightDir));
	if(s < 0.00001) return vec3(0.0, 0.0, 0.0);
	return ((lightPower*s)/(4.f*float(M_PI)*dist*dist));
	
}
vec3 DirectLight(const vec3& i){
	vec3 lightDir = lightPos-i;
	float dist = glm::length(lightDir);
	float s = glm::dot(currentNormal, glm::normalize(lightDir));
	if(s < 0.00001 ) return vec3(0.0, 0.0, 0.0);
	if(USE_SHADOWS && Shadow(i, lightDir) < dist) return vec3(0.0, 0.0, 0.0);
	return ((lightPower*s)/(4.f*float(M_PI)*dist*dist));
	
}
float Shadow(vec3 start, vec3 dir){
	float distance = numeric_limits<float>::max();
	for(int i=0; i<triangles.size(); ++i){
		vec3 e1 = triangles[i].v1 - triangles[i].v0;
		vec3 e2 = triangles[i].v2 - triangles[i].v0;
		vec3 b = start - triangles[i].v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = glm::inverse( A ) * b;
		if(0.00001 < x.x  && 0.0 <= x.y && 0.0 <= x.z && x.y+x.z <= 1 && x.x < distance){
			distance = x.x;
		}
	}
	return distance;
}

void VertexShader(const Vertex & v, Pixel & p){
	if(DEBUG) cerr << "VertexShader: (" << v.position.x << ", " << v.position.y << ", " << v.position.z << ")" << endl;
	vec3 prim = (v.position-cameraPos)*R;
	p.x = focalLength * prim.x/prim.z + SCREEN_WIDTH/2;
	p.y = focalLength * prim.y/prim.z + SCREEN_HEIGHT/2;
	p.zinv = 1/double(prim.z);
	//p.illumination = v.reflectance * (indirectLightPower + DirectLight(v));
	p.pos3d = v.position*float(p.zinv);
}

void Interpolate( Pixel a, Pixel b, vector<Pixel>& result ){
	float size = result.size()-1;
	if(DEBUG) cerr << "Interpolate: (" << a.x << ", " << a.y << ") to (" << b.x << ", " << b.y << ") in " << size+1 << "steps" << endl;
	float stepX = (b.x-a.x) / max(size, 1.f);
	float stepY = (b.y-a.y) / max(1.f, size);
	float stepZinv = (b.zinv-a.zinv) / max(size, 1.f);
	//vec3 stepIllum = (b.illumination-a.illumination) / max(size, 1.f);
	vec3 stepPos = (b.pos3d-a.pos3d) / max(size, 1.f);
	Pixel current = a;
	for (size_t i = 0; i < size+1; i++){
		result[i].x = int(current.x+i*stepX);
		result[i].y = int(current.y+i*stepY);
		result[i].zinv = double(current.zinv+i*stepZinv);
		//result[i].illumination = current.illumination+float(i)*stepIllum;
		result[i].pos3d = a.pos3d+float(i)*stepPos;
		

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

void DrawPolygonEdges(const vector<Vertex> & vertices){
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

void DrawDepthBuffer(){

	for (int i = 0; i < SCREEN_WIDTH; i++){
		for (int j = 0; j < SCREEN_HEIGHT; j++){
			double col = depthBuffer[i][j];
			PutPixelSDL(screen, i, j, vec3(col, col, col));
		}
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

void PixelShader(Pixel & p){
	if(p.x < 0 || p.x >= SCREEN_WIDTH || p.y < 0 || p.y >= SCREEN_HEIGHT) return;
	if(p.zinv + 0.0001 > depthBuffer[p.x][p.y]){
		p.illumination = currentReflectance * (indirectLightPower + DirectLight(p.pos3d/float(p.zinv)));
		PutPixelSDL(screen, p.x, p.y, p.illumination);
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

		for(vector<Pixel>::iterator it = line.begin(); it != line.end(); it++){
	//		cerr << *it << endl;
			PixelShader(*it);
		}
	}
	
}

void DrawPolygon( const vector<Vertex>& vertices ){
	int V = vertices.size();
	vector<Pixel> vertexPixels(V);
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
