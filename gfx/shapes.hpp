#ifndef INO_GFX_SHAPE_INCLUDED
#define INO_GFX_SHAPE_INCLUDED

#include "../dx/dx.hpp"
#include "../dx/dx12resource.hpp"

namespace ino::shape
{

d3d::vbo CreateQuad();
d3d::vbo CreateCube();

}

#endif