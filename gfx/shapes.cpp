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

d3d::StaticMesh CreateTestModel()
{
	namespace DX = DirectX;
	std::vector<DX::XMVECTOR> vert;
	std::vector<uint32_t> idx;
	UINT32 i;
	//A Panel
	vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	idx.push_back(i); idx.push_back(i+1);	idx.push_back(i+2);
	idx.push_back(i + 2); idx.push_back(i + 3); idx.push_back(i); 
	i += 4;
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-5, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	idx.push_back(i); idx.push_back(i + 1);	idx.push_back(i + 2);
	idx.push_back(i + 2); idx.push_back(i + 3); idx.push_back(i);
	i += 4;
	//B panel
	vert.push_back(DX::XMVectorSet(-1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, 1, 0, 1)); vert.push_back(DX::XMVectorSet(0, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-3, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	vert.push_back(DX::XMVectorSet(-1, -0.3, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
	idx.push_back(i); idx.push_back(i + 1);	idx.push_back(i + 2);
	idx.push_back(i + 2); idx.push_back(i + 3); idx.push_back(i);
	i += 4;

	//curve
	for(int j = 0;j < 5;++j)
	{
		float x = sinf( DX::XM_PI *(j/4.0) );
		float y = cosf( DX::XM_PI * (j / 4.0));

		vert.push_back(DX::XMVectorSet(-3-x, 1, y, 1)); vert.push_back(DX::XMVectorSet(1, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(-5-x, 1, y, 1)); vert.push_back(DX::XMVectorSet(0, 0, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(-5-x, -0.3, y, 1)); vert.push_back(DX::XMVectorSet(0, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		vert.push_back(DX::XMVectorSet(-3-x, -0.3, y, 1)); vert.push_back(DX::XMVectorSet(1, 1, 0, 1)); vert.push_back(DX::XMVectorSet(1, 1, 1, 1));
		idx.push_back(i); idx.push_back(i + 1);	idx.push_back(i + 2);
		idx.push_back(i + 2); idx.push_back(i + 3); idx.push_back(i);
		i += 4;
	}

	d3d::StaticMesh mesh;
	mesh.vbo.Create(&vert[0], 12 * sizeof(float), sizeof(DX::XMVECTOR)*vert.size());
	mesh.ibo.Create(&idx[0], sizeof(uint32_t)*idx.size());

	return mesh;
}

}