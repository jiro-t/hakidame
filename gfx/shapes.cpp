#include "shapes.hpp"

#include <DirectXMath.h>
#include <vector>

namespace ino::shape
{

d3d::StaticMesh CreateQuad()
{
	namespace DX = DirectX;
	static const DX::XMVECTOR vert[] = {
		DX::XMVectorSet( 1, 1,0,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet(-1, 1,0,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet(-1,-1,0,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet( 1,-1,0,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1)
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
		DX::XMVectorSet( 1, 1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,-1,1),
		//back
		DX::XMVectorSet( 1, 1,1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,1,1),
		DX::XMVectorSet( 1,-1,1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,1,1),
		DX::XMVectorSet(-1,-1,1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,1,1),
		DX::XMVectorSet(-1, 1,1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,0,1,1),
		//top
		DX::XMVectorSet(-1,1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,1,0,1),
		DX::XMVectorSet(-1,1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,1,0,1),
		DX::XMVectorSet( 1,1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,1,0,1),
		DX::XMVectorSet( 1,1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,1,0,1),
		//bottom
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,-1,0,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,-1,0,1),
		DX::XMVectorSet( 1,-1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,-1,0,1),
		DX::XMVectorSet( 1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(0,-1,0,1),
		//left
		DX::XMVectorSet(-1, 1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(-1,0,0,1),
		DX::XMVectorSet(-1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(-1,0,0,1),
		DX::XMVectorSet(-1,-1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(-1,0,0,1),
		DX::XMVectorSet(-1, 1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(-1,0,0,1),
		//right
		DX::XMVectorSet(1, 1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(1,0,0,1),
		DX::XMVectorSet(1,-1,-1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(1,0,0,1),
		DX::XMVectorSet(1,-1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(1,0,0,1),
		DX::XMVectorSet(1, 1, 1,1),DX::XMVectorSet(1,1,1,1),DX::XMVectorSet(1,0,0,1),
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

d3d::StaticMesh CreateSphere(DirectX::XMVECTOR color1, DirectX::XMVECTOR color2)
{
	namespace DX = DirectX;
	std::vector<DX::XMVECTOR> vert;
	std::vector<uint32_t> idx;
	static const float step = DirectX::XM_PI / 8.0;
	int ci = 0;
	int cj = 0;
	int i = 0;
	for (float phi = 0.f; phi <= DirectX::XM_2PI; phi += step)
	{
		for (float theta = 0.f; theta <= DirectX::XM_2PI; theta += step)
		{
			float x = cosf(theta)*cosf(phi);
			float z = cosf(theta)*sinf(phi);
			float y = sinf(theta);
			float x2 = cosf(theta+step) * cosf(phi);
			float z2 = cosf(theta+step) * sinf(phi);
			float y2 = sinf(theta+step);
			float x3 = cosf(theta + step) * cosf(phi + step);
			float z3 = cosf(theta + step) * sinf(phi + step);
			float x4 = cosf(theta) * cosf(phi + step);
			float z4 = cosf(theta) * sinf(phi + step);

			auto norm = DX::XMVector3Cross(
				DX::XMVectorSubtract(DX::XMVectorSet(x2, y, z2, 1),DX::XMVectorSet(x, y, z,1)),
				DX::XMVectorSubtract(DX::XMVectorSet(x2, y, z2, 1),DX::XMVectorSet(x, y2, z,1)) );
			DX::XMVECTOR color = (ci+cj)%2 == 0 ? color1 : color2;
			ci = (ci + 1) % 2;/*DX::XMVector3Normalize(norm)*/
			vert.push_back(DX::XMVectorSet(x , y,  z,1));vert.push_back(color); vert.push_back(color);
			vert.push_back(DX::XMVectorSet(x2,y2, z2,1));vert.push_back(color); vert.push_back(color);
			vert.push_back(DX::XMVectorSet(x3,y2, z3,1));vert.push_back(color); vert.push_back(color);
			vert.push_back(DX::XMVectorSet(x4, y, z4,1));vert.push_back(color); vert.push_back(color);

			idx.push_back(i + 0); idx.push_back(i + 1); idx.push_back(i + 2); 
			idx.push_back(i + 2); idx.push_back(i + 3); idx.push_back(i + 0);
			i = vert.size();
		}
		cj++;
	}

	d3d::StaticMesh mesh;
	mesh.vbo.Create(&vert[0], 3 * sizeof(DX::XMVECTOR), vert.size()*sizeof(DX::XMVECTOR));
	mesh.ibo.Create(&idx[0], idx.size()*sizeof(uint32_t));

	return mesh;
}

}