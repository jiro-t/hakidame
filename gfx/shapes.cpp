#include "shapes.hpp"

#include <DirectXMath.h>

namespace ino::shape
{

d3d::vbo CreateQuad()
{
	namespace DX = DirectX;
	static const DX::XMVECTOR vert[] = {
		DX::XMVectorSet( 1, 1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1, 1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1)
	};
	d3d::vbo vbo;
	vbo.Create(vert, 12 * sizeof(float), sizeof(vert));
	return vbo;
}

d3d::vbo CreateCube()
{
	namespace DX = DirectX;
	static const DX::XMVECTOR vert[] = {
		//front
		DX::XMVectorSet( 1, 1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1, 1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		//back
		DX::XMVectorSet( 1, 1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1, 1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		//top
		DX::XMVectorSet(-1,1,-1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,1, 1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,1, 1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,1, 1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,1,-1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,1,-1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		//bottom
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1, 1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1, 1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		//left
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1, 1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		//right
		DX::XMVectorSet(1, 1,-1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1,-1,-1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1,-1, 1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1,-1, 1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1, 1, 1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1, 1,-1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1)
	};
	d3d::vbo vbo;
	vbo.Create(vert, 12 * sizeof(float), sizeof(vert));
	return vbo;
}

d3d::vbo CreateStaticMesh(){ d3d::vbo vbo; return vbo; }

}