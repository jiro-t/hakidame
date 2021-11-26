#ifndef INO_GFX_SHAPE_INCLUDED
#define INO_GFX_SHAPE_INCLUDED

#include "../dx/dx.hpp"
#include "../dx/dx12resource.hpp"

namespace ino::shape
{

d3d::StaticMesh CreateQuad();
d3d::StaticMesh CreateCube();

d3d::StaticMesh CreateSphere(DirectX::XMVECTOR color1, DirectX::XMVECTOR color2);

}

#endif