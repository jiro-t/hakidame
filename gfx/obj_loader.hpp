#ifndef INO_GRAPHICS_OBJ_LOADER_HPP
#define INO_GRAPHICS_OBJ_LOADER_HPP

#include <DirectXMath.h>
#include <vector>

#include "../dx/dx12resource.hpp"

namespace ino::gfx::obj{

struct material_t
{
	float Ns;
	float Ni;
	float d;//アルファ値(透過率)
	float Tr;
 
	uint32_t illum;
	DirectX::XMFLOAT4 Ka;//環境光反射成分
	DirectX::XMFLOAT4 Kd;//拡散反射成分
	DirectX::XMFLOAT4 Ks;//鏡面反射成分
	DirectX::XMFLOAT4 Ke;
};

struct index_t{
	std::vector<uint32_t> id_p;
	std::vector<uint32_t> id_t;
	std::vector<uint32_t> id_n;
	std::vector<uint32_t> count;

	std::vector<uint32_t> mat_id;
	std::vector<uint32_t> mat_count;

	index_t();
};

struct vertex_t{
	std::vector<DirectX::XMFLOAT4> v_p;
	std::vector<DirectX::XMFLOAT2> v_t;
	std::vector<DirectX::XMFLOAT4> v_n;

	vertex_t();
};

void parse_obj_mesh(
					std::istream & ifs
				  ,	std::vector<DirectX::XMFLOAT4> & p
				  ,	std::vector<DirectX::XMFLOAT2> & t
				  ,	std::vector<DirectX::XMFLOAT4> & n
				  ,	index_t & i);
//--material
std::vector<material_t> load_obj_mtl(std::istream& ifs);
std::vector<material_t> load_obj_mtl(void *mem, uint32_t size);
//--mesh
ino::d3d::StaticMesh load_obj(std::istream& ifs);
ino::d3d::StaticMesh load_obj(void* mem,uint32_t size);

//GL_T2F_N3F_V3F
namespace mesh_detail{
constexpr unsigned int size_position = 4;
constexpr unsigned int size_normal = 4;
constexpr unsigned int size_texcoord = 2;

//static const unsigned int offset_position = sizeof(float)*2 + sizeof(float)*3;
//static const unsigned int offset_normal = sizeof(float)*2;
//static const unsigned int offset_texcoord = 0;
constexpr unsigned int vert_stride = sizeof(float)*(4 + 4 + 4);

}

}


#endif

