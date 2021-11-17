#ifndef GGRKS_GRAPHICS_OBJ_LOADER_HPP
#define GGRKS_GRAPHICS_OBJ_LOADER_HPP

#include <boost/cstdint.hpp>
//#define GLM_MESSAGES
#include <glm/glm.hpp>
#include <ggrks/opengl/gl_buffer_array.hpp>
#include <ggrks/opengl/gl_texture2d.hpp>

namespace ggrks { namespace graphics { namespace obj{

struct material_t
{
	std::string material_name;

	float Ns;
	float Ni;
	float d;//アルファ値(透過率)
	float Tr;
	std::vector<glm::vec4> Tf; 
	boost::uint32_t illum;
	glm::vec4 Ka;//環境光反射成分
	glm::vec4 Kd;//拡散反射成分
	glm::vec4 Ks;//鏡面反射成分
	glm::vec4 Ke;

	gl::texture2d map_Kd;
//	gl::texture2d bump;

	bool is_mapped_Kd;

	boost::gil::rgb8_image_t map_Kd_src;
//	boost::gil::rgb8_image_t bump_src;
};

struct index_t{
	std::vector<boost::uint32_t> id_p;
	std::vector<boost::uint32_t> id_t;
	std::vector<boost::uint32_t> id_n;
	std::vector<boost::uint32_t> count;

	std::vector<boost::uint32_t> mat_id;
	std::vector<boost::uint32_t> mat_count;

	index_t();
};

struct vertex_t{
	std::vector<glm::vec4> v_p;
	std::vector<glm::vec2> v_t;
	std::vector<glm::vec4> v_n;

	vertex_t();
};

void parse_obj_mesh(
					std::ifstream & ifs
				  ,	std::vector<glm::vec4> & p
				  ,	std::vector<glm::vec2> & t
				  ,	std::vector<glm::vec4> & n
				  ,	index_t & i
				  , std::vector<material_t> const & mat_list = std::vector<material_t>());
//--material
std::vector<material_t> load_obj_mtl(std::wstring const & path);
//--mesh
gl::gl_static_mesh load_obj(std::wstring path,GLenum usage = GL_STATIC_DRAW);
std::vector<gl::gl_static_mesh> load_obj(std::wstring path,std::vector<material_t> const & mat_list,GLenum usage = GL_STATIC_DRAW);

//GL_T2F_N3F_V3F
namespace mesh_detail{
static const unsigned int size_position = 3;
static const unsigned int size_normal = 3;
static const unsigned int size_texcoord = 2;

static const unsigned int offset_position = sizeof(float)*2 + sizeof(float)*3;
static const unsigned int offset_normal = sizeof(float)*2;
static const unsigned int offset_texcoord = 0;

static const unsigned int vert_stride = sizeof(float)*(2 + 3 + 3);
}

}}}


#endif

