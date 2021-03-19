#ifndef INO_GRAPHICS_OBJ_INCLUDED
#define INO_GRAPHICS_OBJ_INCLUDED

#include "../dx/dx12resource.hpp"
#include <DirectXMath.h>

#include <fstream>
#include <vector>

namespace ino::graphics
{

class modelInterface {
public:
	virtual void Draw() = 0;
};

class material {
	ino::d3d::texture tex;
	DirectX::XMVECTOR color;
public:
	material() {}
};

class objModel : public modelInterface
{
	std::vector<ino::d3d::vbo> vbo;
public:
	objModel() { ; }

	void Create(std::wstring const & path) 
	{
		std::ifstream ifs(path);

		char str[256];
		ifs.getline(str,256);
		if(str[0] == 'f')//vertex
		{
		}
		if (str[0] == 'v')//index
		{
			float a,b,c;
			sscanf_s(str+1,"%f,%f,%f",&a,&b,&c);
		}
	}
};

}

#endif