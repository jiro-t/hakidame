#ifndef INO_GFX_FONT_INCLUDED
#define INO_GFX_FONT_INCLUDED

#include "../dx/dx.hpp"
#include "../dx/dx12resource.hpp"

namespace ino::shape
{

d3d::StaticMesh CreateCharMesh(wchar_t c,LPCWSTR font);

}

#endif