#include "shapes.hpp"

#include <DirectXMath.h>

namespace ino::shape
{

d3d::StaticMesh CreateQuad()
{
	namespace DX = DirectX;
	static const DX::XMVECTOR vert[] = {
		DX::XMVectorSet( 1, 1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,0,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1)
	};
	static const uint32_t idx[] = { 0,1,2,2,3,0 };

	d3d::StaticMesh mesh;
	mesh.vbo.Create(vert, 12 * sizeof(float), sizeof(vert));
	mesh.ibo.Create(idx,sizeof(idx));

	return mesh;
}

d3d::StaticMesh CreateCube()
{
	namespace DX = DirectX;
	static const DX::XMVECTOR vert[] = {
		//front
		DX::XMVectorSet( 1, 1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(0,0,-1,1),DX::XMVectorSet(1,1,1,1),
		//back
		DX::XMVectorSet( 1, 1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1,1,1),DX::XMVectorSet(0,0,1,1),DX::XMVectorSet(1,1,1,1),
		//top
		DX::XMVectorSet(-1,1,-1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,1, 1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,1, 1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,1,-1,1),DX::XMVectorSet(0,1,0,1),DX::XMVectorSet(1,1,1,1),
		//bottom
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1, 1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(0,-1,0,1),DX::XMVectorSet(1,1,1,1),
		//left
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(-1, 1, 1,1),DX::XMVectorSet(-1,0,0,1),DX::XMVectorSet(1,1,1,1),
		//right
		DX::XMVectorSet(1, 1,-1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1,-1,-1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1,-1, 1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
		DX::XMVectorSet(1, 1, 1,1),DX::XMVectorSet(1,0,0,1),DX::XMVectorSet(1,1,1,1),
	};

	static const uint32_t idx[] = { 
		0,1,2,2,3,0,
		4,5,6,6,7,4,
		8,9,10,10,11,8,
		12,13,14,14,15,12,
		16,17,18,18,19,16,
		20,21,22,22,23,20
	};

	d3d::StaticMesh mesh;
	mesh.vbo.Create(vert, 12 * sizeof(float), sizeof(vert));
	mesh.ibo.Create(idx, sizeof(idx));

	return mesh;
}

}