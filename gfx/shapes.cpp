#include "shapes.hpp"

#include <DirectXMath.h>
#include <vector>

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

void PushLaneWall(DirectX::XMVECTOR scale, DirectX::XMVECTOR offset,UINT32 &i,std::vector<DirectX::XMVECTOR>& vert, std::vector<uint32_t>& id)
{
	//panel A
	namespace DX = DirectX;

	UINT32 begin = vert.size();
	//A Panel Front
	vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	//A Panel Back
	vert.push_back(DX::XMVectorSet(1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//B panel
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//curve
	for (int j = 0; j < 8; ++j)
	{
		float x = -sinf(DX::XM_PI * (j / 8.f));
		float y = -cosf(DX::XM_PI * (j / 8.f)) + 1;
		float xn = -sinf(DX::XM_PI * ((j+1.f)/8.f));
		float yn = -cosf(DX::XM_PI * ((j+1.f)/8.f)) + 1;
		//float t = (j % 5) / 4.f;
		DX::XMVECTOR nv = DX::XMVectorSet(yn - y, 0, x - xn, 1);
		vert.push_back(DX::XMVectorSet(1 - x, 1, y, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - xn, 1, yn, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - xn, -0.3, yn, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - x, -0.3, y, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
		i += 4;
	}

	for (UINT32 j = begin; j < vert.size(); j += 3)
	{
		vert[j] = DX::XMVectorMultiply(vert[j],scale);
		vert[j] = DX::XMVectorAdd(vert[j],offset);
	}
}

void PushLaneFloor(DirectX::XMVECTOR scale, DirectX::XMVECTOR offset, UINT32& i, std::vector<DirectX::XMVECTOR>& vert, std::vector<uint32_t>& id)
{
	//panel A
	namespace DX = DirectX;

	UINT32 begin = vert.size();
	//A Panel Front
	vert.push_back(DX::XMVectorSet(1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	//A Panel Back
	vert.push_back(DX::XMVectorSet(1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//B panel
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, -1, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 3, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//curve
	for (int j = 0; j < 8; ++j)
	{
		float x = -sinf(DX::XM_PI * (j / 8.f));
		float y = -cosf(DX::XM_PI * (j / 8.f)) + 1;
		float xn = -sinf(DX::XM_PI * ((j + 1.f) / 8.f));
		float yn = -cosf(DX::XM_PI * ((j + 1.f) / 8.f)) + 1;

		float x2 = -sinf(DX::XM_PI * (j / 8.f))*2;
		float y2 = -cosf(DX::XM_PI * (j / 8.f))*2 + 1;
		float xn2 = -sinf(DX::XM_PI * ((j + 1.f) / 8.f))*2;
		float yn2 = -cosf(DX::XM_PI * ((j + 1.f) / 8.f))*2 + 1;
		//float t = (j % 5) / 4.f;
		DX::XMVECTOR nv = DX::XMVectorSet(0,1,0,1);
		vert.push_back(DX::XMVectorSet(1 - x , -0.3, y, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - x2, -0.3, y2, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - xn2, -0.3, yn2, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 - xn , -0.3, yn, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
		i += 4;
	}

	for (UINT32 j = begin; j < vert.size(); j += 3)
	{
		vert[j] = DX::XMVectorMultiply(vert[j], scale);
		vert[j] = DX::XMVectorAdd(vert[j], offset);
	}
}

d3d::StaticMesh CreateTestModel()
{
	//panel A
	namespace DX = DirectX;

	std::vector<DX::XMVECTOR> vert;
	std::vector<uint32_t> id;
	UINT32 i = 0;
	//A Panel Front
	vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	//A Panel Back
	vert.push_back(DX::XMVectorSet(1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//B panel
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;
	vert.push_back(DX::XMVectorSet(-1, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, 1, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 2, 1)); vert.push_back(DX::XMVectorSet(0, 0, -1, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
	i += 4;

	//curve
	for(int j = 0;j < 8;++j)
	{
		float x = -sinf(DX::XM_PI *(j/8.f));
		float y = -cosf(DX::XM_PI *(j/8.f))+1;
		float xn = -sinf(DX::XM_PI * ((j+1.f) / 8.f));
		float yn = -cosf(DX::XM_PI * ((j+1.f) / 8.f))+1;
		//float t = (j % 5) / 4.f;
		DX::XMVECTOR nv = DX::XMVectorSet(yn - y,0, x - xn,1);
		vert.push_back(DX::XMVectorSet(1 -x, 1, y, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 -xn, 1, yn, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 -xn, -0.3, yn, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(1 -x, -0.3, y, 1)); vert.push_back(nv); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		id.push_back(i); id.push_back(i + 1); id.push_back(i + 2); id.push_back(i + 2); id.push_back(i + 3); id.push_back(i);
		i += 4;
	}

	PushLaneWall(DX::XMVectorSet(1.2,0.1,1.8,1), DX::XMVectorSet(0.5,-0.3,-0.8,0),i,vert,id);
	PushLaneFloor(DX::XMVectorSet(1, 1, 1, 1), DX::XMVectorSet(0, 0, 0, 0), i, vert, id);
	//PushLaneFloor(DX::XMVectorSet(1.2, 1, 1.8, 1), DX::XMVectorSet(0.5, -0.3, -0.8, 0), i, vert, id);

	//bottom

	d3d::StaticMesh mesh;
	mesh.vbo.Create(&(vert[0]), 12 * sizeof(float), sizeof(DX::XMVECTOR)*vert.size());
	mesh.ibo.Create(&(id[0]), sizeof(uint32_t)*id.size());

	return mesh;
}

}