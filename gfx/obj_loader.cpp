#include <algorithm>
#include <fstream>

#include "graphics/obj_loader.hpp"
/*
namespace ino { namespace graphics { namespace obj{

static const boost::uint32_t max_line_length = 1024;
static const boost::uint32_t init_vert_length = 5096;

void read_v(std::string const& tag,std::string const& s,std::vector< glm::vec4 >& vec)
{
	namespace fusion = boost::fusion;
	namespace qi = boost::spirit::qi;

	typedef fusion::tuple<float,float,float> value_type;
	value_type result;
	if( qi::phrase_parse(
					s.begin()
				,	s.end()
				,	tag >> ( qi::float_ >> qi::float_ >> qi::float_ )
				,	qi::ascii::space
				,	result ))
	{
		vec.push_back( 
			glm::vec4(
				 static_cast<float>(fusion::at_c<0>(result))
				,static_cast<float>(fusion::at_c<1>(result))
				,static_cast<float>(fusion::at_c<2>(result))
				,0.f
			)
		);
	}
}

void read_vt(std::string const& tag,std::string const& s,std::vector< glm::vec2 >& vec)
{
	namespace fusion = boost::fusion;
	namespace qi = boost::spirit::qi;

	typedef fusion::tuple<float,float,float> value_type;
	value_type result;
	if( qi::phrase_parse(
					s.begin()
				,	s.end()
				,	tag >> ( qi::float_ >> qi::float_ )
				,	qi::ascii::space
				,	result ))
	{
		vec.push_back( 
			glm::vec2(
				 static_cast<float>(fusion::at_c<0>(result))
				,static_cast<float>(fusion::at_c<1>(result))
			)
		);
	}
}

void read_i(std::string const& s,index_t& data)
{
	namespace qi = boost::spirit::qi;
	namespace fusion = boost::fusion;

	typedef fusion::tuple<boost::uint32_t,boost::uint32_t,boost::uint32_t> value_type;
	typedef std::vector< value_type > result_type;
	result_type result;
	if( qi::phrase_parse( //face is [pos/tex/norm]
				s.begin()
			,	s.end()
			,	"f" >> *( qi::uint_ >> "/" >> qi::uint_ >> "/" >> qi::uint_)
			,	qi::ascii::space
			,	result 
		))
	{
		boost::for_each(result
			, [&data](value_type & value){
				data.id_p.push_back( static_cast<boost::uint32_t>(fusion::at_c<0>(value)) - 1 );
				data.id_t.push_back( static_cast<boost::uint32_t>(fusion::at_c<1>(value)) - 1 );
				data.id_n.push_back( static_cast<boost::uint32_t>(fusion::at_c<2>(value)) - 1 );
			}
		);
		data.count.push_back( result.size() );

		if( data.mat_count.size() )
			data.mat_count.back() += result.size();
	}
}

glm::vec4 read_f3(std::string const& tag,std::string const& s)
{
	namespace fusion = boost::fusion;
	namespace qi = boost::spirit::qi;

	typedef fusion::tuple<float,float,float> value_type;
	value_type result;
	if( qi::phrase_parse(
					s.begin()
				,	s.end()
				,	tag >> ( qi::float_ >> qi::float_ >> qi::float_ )
				,	qi::ascii::space
				,	result ))
	{
		return glm::vec4(
			 static_cast<float>(fusion::at_c<0>(result))
			,static_cast<float>(fusion::at_c<1>(result))
			,static_cast<float>(fusion::at_c<2>(result))
			,0.f
		);
	}
}

boost::gil::rgb8_image_t read_texture(std::string const & path)
{
	namespace gil = boost::gil;
	if (path.find(".png", 0) != std::wstring::npos)
	{
		gil::rgb8_image_t img;
		gil::png_read_image(path, img);
		return img;
	}
	
	::exit(1);
}

//material
material_t create_mat(std::ifstream & ifs,std::string const & name)
{
	material_t ret;
	ret.material_name = name;

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
			ret.map_Kd_src = read_texture(path);
			ret.map_Kd.load( ret.map_Kd_src );
			ret.is_mapped_Kd = true;
			std::cout << "read_texture:" << path << std::endl;
		}
		else if( std::string(line+1,line+1+5) == "map_d" );
		else if( std::string(line+1,line+1+5) == "bump" ||
				 std::string(line+1,line+1+5) == "map_bump" );
		else
			return ret;
	}
	return ret;
}

std::vector<material_t> load_obj_mtl(std::wstring const & path)
{
	std::ifstream ifs(to_string(path));

	if( ifs.fail() )
		std::cout << "can't read file :" << path.c_str() << std::endl;

	std::vector<material_t> ret;
	ret.reserve(100);

	char line[max_line_length] = "";
	while( !ifs.eof() )
	{
		ifs.getline( line,max_line_length );
		if( std::string(line,line+6) == "newmtl" )
		{
			//std::cout << "-material:" << std::string(line + 7) << std::endl;
			ret.push_back( create_mat(ifs,std::string(line + 7)) );
		}
		else
			continue;
	}

	return ret;
}

//mesh
void parse_obj_mesh(
					std::ifstream & ifs
				  ,	std::vector<glm::vec4> & p
				  ,	std::vector<glm::vec2> & t
				  ,	std::vector<glm::vec4> & n
				  ,	index_t & i
				  , std::vector<material_t> const & mat_list )
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
			std::string tag(line + 7);
			for(std::size_t m = 0;m < mat_list.size();++m)
			{
				if(tag == mat_list[m].material_name)
				{
					i.mat_id.push_back(m);
					i.mat_count.push_back(0);
				}
			}
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

static void push_f4(std::vector<float>&v,glm::vec4 const & rhs)
{
	v.push_back(rhs[0]);
	v.push_back(rhs[1]);
	v.push_back(rhs[2]);
	v.push_back(rhs[3]);
}

static void push_f3(std::vector<float>&v,glm::vec4 const & rhs)
{
	v.push_back(rhs[0]);
	v.push_back(rhs[1]);
	v.push_back(rhs[2]);
}

static void push_f2(std::vector<float>&v,glm::vec2 const & rhs)
{
	v.push_back(rhs[0]);
	v.push_back(1.f -rhs[1]);
}

gl::gl_static_mesh load_obj(std::wstring path,GLenum usage)
{
	std::ifstream ifs(to_string(path));

	if( ifs.fail() )
		std::cout << "can't read file :" << path.c_str() << std::endl;

	vertex_t tmp_vert;
	index_t tmp_idx;

	parse_obj_mesh(ifs,tmp_vert.v_p,tmp_vert.v_t,tmp_vert.v_n,tmp_idx);

	std::cout << "v :" << tmp_vert.v_p.size() << std::endl;
	//GL_T2F_N3F_V3F
	std::vector<float> gl_vert;
	gl_vert.reserve(init_vert_length*3+init_vert_length*2+init_vert_length*3);
	std::vector<GLuint> triangulated_index;

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
				std::cout << "out of range about triangulate :" << *it_cnt << std::endl;
			}
			++it_cnt;
		}
	}

	return gl::gl_static_mesh{
			gl::vertex_buffer_array(gl_vert.size()*sizeof(float),usage,(float*)&gl_vert[0])
		,	gl::index_buffer_array(triangulated_index.size()*sizeof(GLuint),usage,(unsigned*)&triangulated_index[0])
		,	0
	};
}

std::vector<gl::gl_static_mesh> load_obj(std::wstring path,std::vector<material_t> const & mat_list,GLenum usage)
{
	std::vector<gl::gl_static_mesh> ret;
	ret.reserve( mat_list.size() );
	std::ifstream ifs(to_string(path));

	if( ifs.eof() )
		std::cout << "can't read file :" << path.c_str() << std::endl;

	vertex_t tmp_vert;
	index_t tmp_idx;

	parse_obj_mesh(ifs,tmp_vert.v_p,tmp_vert.v_t,tmp_vert.v_n,tmp_idx,mat_list);

	std::cout << "v :" << tmp_vert.v_p.size() << std::endl;
	//GL_T2F_N3F_V3F
	std::vector<float> gl_vert;
	gl_vert.reserve(init_vert_length*3+init_vert_length*2+init_vert_length*3);
	std::vector<GLuint> triangulated_index;
	triangulated_index.reserve(init_vert_length*3);

	//create triangulate index array
	{
		triangulated_index.reserve(init_vert_length);
		auto it_cnt  = tmp_idx.count.begin();
		auto it_idp  = tmp_idx.id_p.begin();
		auto it_idn  = tmp_idx.id_n.begin();
		auto it_idt  = tmp_idx.id_t.begin();

		auto it_mat_id  = tmp_idx.mat_id.begin();
		auto it_mat_c  = tmp_idx.mat_count.begin();
		std::size_t cnt_mat = 0;

		while(it_cnt != tmp_idx.count.end())
		{
			if(*it_cnt == 3)
			{
				for(std::size_t i = 0;i < *it_cnt;++i,++it_idp,++it_idn,++it_idt)
				{
					triangulated_index.push_back( triangulated_index.size() );

					push_f3( gl_vert,tmp_vert.v_p[*it_idp] );
					push_f3( gl_vert,tmp_vert.v_n[*it_idn] );
					push_f4( gl_vert,mat_list[*it_mat_id].Ka );
					push_f2( gl_vert,tmp_vert.v_t[*it_idt] );
				}

				cnt_mat += 3;
			}
			else if(*it_cnt == 4)
			{
				const std::size_t tris[6] = {0,2,1,0,3,2};
				for(auto e : tris)
				{
					push_f3( gl_vert,tmp_vert.v_p[ it_idp[e] ] );
					push_f3( gl_vert,tmp_vert.v_n[ it_idn[e] ] );
					push_f4( gl_vert,mat_list[*it_mat_id].Ka );
					push_f2( gl_vert,tmp_vert.v_t[ it_idt[e] ] );
					triangulated_index.push_back( triangulated_index.size() );
				}

				cnt_mat += 4;
				it_idp += 4; it_idn += 4; it_idt += 4;
			}
			else
			{
				std::cout << "out of range about triangulate :" << *it_cnt << std::endl;
			}

			if(*it_mat_c <= cnt_mat)
			{
				ret.push_back( gl::gl_static_mesh{
							gl::vertex_buffer_array(gl_vert.size()*sizeof(float),usage,(float*)&gl_vert[0])
						,	gl::index_buffer_array(triangulated_index.size()*sizeof(GLuint),usage,(unsigned*)&triangulated_index[0])
						,	*it_mat_id
				});
				cnt_mat = 0;
				++it_mat_id;
				++it_mat_c;

				triangulated_index.clear();
				gl_vert.clear();
			}

			++it_cnt;
		}
	}

	return ret;
}

}}}
*/