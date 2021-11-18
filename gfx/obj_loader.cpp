#include <algorithm>
#include <fstream>

#include "obj_loader.hpp"

namespace ino::gfx::obj{

static const uint32_t max_line_length = 1024;
static const uint32_t init_vert_length = 5096;

void read_v(std::string const& tag,std::string const& s,std::vector<DirectX::XMFLOAT4>& vec)
{
	std::string format = tag + "%f %f %f";
	DirectX::XMFLOAT4 x;
	sscanf_s(s.c_str(), format.c_str(), &x.x, &x.y, &x.z);
	vec.push_back(x);
}

void read_vt(std::string const& tag,std::string const& s,std::vector<DirectX::XMFLOAT2>& vec)
{
	std::string format = tag + "%f %f";
	DirectX::XMFLOAT2 x;
	sscanf_s(s.c_str(), format.c_str(), &x.x, &x.y);
	vec.push_back(x);
}

void read_i(std::string const& s,index_t& data)
{
	std::string format = "f %d/%d/%d %d/%d/%d %d/%d/%d";
	uint32_t p[3],t[3],n[3];
	sscanf_s(s.c_str(), format.c_str(),
		&p[0], &t[0] ,&n[0],
		&p[1], &t[1], &n[1], 
		&p[2], &t[2], &n[2] );

	data.count.push_back(3);
	for (int i = 0; i < 3; ++i)
	{
		data.id_p.push_back(p[i]-1);
		data.id_t.push_back(t[i]-1);
		data.id_n.push_back(n[i]-1);
	}
}

//material
material_t create_mat(std::istream& ifs)
{
	material_t ret = {};

	char line[max_line_length] = "";
	while( !ifs.eof() )
	{
		ifs.getline( line,max_line_length );
		if( std::string(line+1,line+1+2) == "Ns" );
		else if( std::string(line+1,line+1+2) == "Ni" );
		else if( std::string(line+1,line+1+1) == "d" );
		else if( std::string(line+1,line+1+2) == "Tr" );
		else if( std::string(line+1,line+1+2) == "Tf" );
		else if (std::string(line + 1, line + 1 + 5) == "illum");
		else if (std::string(line + 1, line + 1 + 2) == "Ka")
			;//			ret.Ka = read_f3("Ka", line);
		else if( std::string(line+1,line+1+2) == "Kd" )
			;//			ret.Kd = read_f3("Kd",line);
		else if( std::string(line+1,line+1+2) == "Ks" )
			;//			ret.Ks = read_f3("Ks",line);
		else if( std::string(line+1,line+1+2) == "Ke" )
			;//			ret.Ke = read_f3("Ke",line);
		else if( std::string(line+1,line+1+6) == "map_Ka" );
		else if( std::string(line+1,line+1+6) == "map_Kd" )
		{
			std::string path(line+8);
			//ret.map_Kd_src = read_texture(path);
			//ret.map_Kd.load( ret.map_Kd_src );
			//ret.is_mapped_Kd = true;
			//std::cout << "read_texture:" << path << std::endl;
		}
		else if( std::string(line+1,line+1+5) == "map_d" );
		else if( std::string(line+1,line+1+5) == "bump" ||
				 std::string(line+1,line+1+5) == "map_bump" );
		else
			return ret;
	}
	return ret;
}

std::vector<material_t> load_obj_mtl(std::istream& ifs)
{
	std::vector<material_t> ret;
	ret.reserve(100);

	char line[max_line_length] = "";
	while( !ifs.eof() )
	{
		ifs.getline(line,max_line_length);
		if( std::string(line,line+6) == "newmtl" )
		{
			//std::cout << "-material:" << std::string(line + 7) << std::endl;
			ret.push_back( create_mat(ifs) );
		}
		else
			continue;
	}

	return ret;
}

//mesh
void parse_obj_mesh(
					std::istream & ifs
				  ,	std::vector<DirectX::XMFLOAT4> & p
				  ,	std::vector<DirectX::XMFLOAT2> & t
				  ,	std::vector<DirectX::XMFLOAT4> & n
				  ,	index_t & i )
{
	char line[max_line_length] = "";
	while( !ifs.eof() )
	{
		ifs.getline( line,max_line_length );
		if(line[0] == 'v')
		{
			if(line[1] == 'n' && line[2] == ' ')
				read_v("vn",line,n);
			else if(line[1] == 't' && line[2] == ' ')
				read_vt("vt",line,t);
			else if(line[1] == ' ')
				read_v("v",line,p);
		}
		else if(line[0] == 'f' && line[1] == ' ')
			read_i(line,i);
		else if( std::string(line,line+6) == "usemtl" )
		{
		}
		else
			continue;
	}
}


index_t::index_t()
{
	id_p.reserve(init_vert_length);
	id_t.reserve(init_vert_length);
	id_n.reserve(init_vert_length);
	count.reserve(init_vert_length / 3);

	mat_id.reserve(100);
	mat_count.reserve(100);
}

vertex_t::vertex_t()
{
	v_p.reserve(init_vert_length);
	v_t.reserve(init_vert_length);
	v_n.reserve(init_vert_length);
}

static void push_f4(std::vector<float>&v, DirectX::XMFLOAT4 const & rhs)
{
	v.push_back(rhs.x);
	v.push_back(rhs.y);
	v.push_back(rhs.z);
	v.push_back(rhs.w);
}

static void push_f3(std::vector<float>&v, DirectX::XMFLOAT4 const & rhs)
{
	v.push_back(rhs.x);
	v.push_back(rhs.y);
	v.push_back(rhs.z);
	v.push_back(1.f);
}

static void push_f2(std::vector<float>&v, DirectX::XMFLOAT2 const & rhs)
{
	v.push_back(rhs.x);
	v.push_back(1.f -rhs.y);
	v.push_back(0.f);
	v.push_back(0.f);
}

ino::d3d::StaticMesh load_obj(std::istream& ifs)
{
	vertex_t tmp_vert;
	index_t tmp_idx;

	parse_obj_mesh(ifs,tmp_vert.v_p,tmp_vert.v_t,tmp_vert.v_n,tmp_idx);
#ifdef _DEBUG
	std::cout << "v :" << tmp_vert.v_p.size() << std::endl;
#endif
	//GL_T2F_N3F_V3F
	std::vector<float> gl_vert;
	gl_vert.reserve(init_vert_length*3+init_vert_length*2+init_vert_length*3);
	std::vector<uint32_t> triangulated_index;

	//create triangulate index array
	{
		triangulated_index.reserve(init_vert_length);
		auto it_cnt  = tmp_idx.count.begin();
		auto it_idp  = tmp_idx.id_p.begin();
		auto it_idn  = tmp_idx.id_n.begin();
		auto it_idt  = tmp_idx.id_t.begin();
		while(it_cnt != tmp_idx.count.end())
		{
			if(*it_cnt == 3)
			{
				for(std::size_t i = 0;i < *it_cnt;++i,++it_idp,++it_idn,++it_idt)
				{
					triangulated_index.push_back( triangulated_index.size() );

					push_f3( gl_vert,tmp_vert.v_p[*it_idp] );
					push_f3( gl_vert,tmp_vert.v_n[*it_idn] );
					push_f2( gl_vert,tmp_vert.v_t[*it_idt] );
				}
			}
			else if(*it_cnt == 4)
			{
				const std::size_t tris[6] = {0,2,1,0,3,2};
				for(std::size_t i = 0;i < 6;++i)
				{
					push_f3( gl_vert,tmp_vert.v_p[ it_idp[tris[i]] ] );
					push_f3( gl_vert,tmp_vert.v_n[ it_idn[tris[i]] ] );
					push_f2( gl_vert,tmp_vert.v_t[ it_idt[tris[i]] ] );
					triangulated_index.push_back( triangulated_index.size() );
				}

				for(std::size_t i = 0;i < 4;++i,++it_idp,++it_idn,++it_idt);
			}
			else
			{
#ifdef _DEBUG
				std::cout << "out of range about triangulate :" << *it_cnt << std::endl;
#endif
			}
			++it_cnt;
		}
	}

	ino::d3d::StaticMesh ret;
	ret.vbo.Create((float*)&gl_vert[0], mesh_detail::vert_stride ,gl_vert.size() * sizeof(float));
	ret.ibo.Create((uint32_t*)&triangulated_index[0], triangulated_index.size() * sizeof(uint32_t));

	return ret;
}

std::vector<material_t> load_obj_mtl(void* mem,uint32_t size) { 
	struct membuf : std::streambuf {
		membuf(char* base, std::ptrdiff_t n) {
			this->setg(base, base, base + n);
		}
	};
	membuf sbuf((char*)mem, size);
	std::istream in(&sbuf);

	return load_obj_mtl(in);
}
ino::d3d::StaticMesh load_obj(void* mem, uint32_t size)
{ 
	struct membuf : std::streambuf {
		membuf(char* base, std::ptrdiff_t n) {
			this->setg(base, base, base + n);
		}
	};
	membuf sbuf((char*)mem, size);
	std::istream in(&sbuf);

	return load_obj(in);
}

}
