#include "dx/dx.hpp"
#include "dx/dx12pipeline.hpp"
#include "dx/dx12resource.hpp"
#include "dx/dx12raytrace.hpp"

#include <thread>
#include <mutex>

float local_time = 0.f;

#include <Windows.h>

//High res time
#include <chrono>
#include <memory>
#include <map>
#include <string>
#include <vector>

#include <chrono>
#include "gfx/font.hpp"
#include "gfx/shapes.hpp"
#include "gfx/obj_loader.hpp"

#include "resource.h"
#include "raster_shader.h"

static float g_Vertices[6][9] = {
	{ 0.30f, 0.f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,0.f }, // 1
	{ 0.3f, 0.55f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,1.f }, // 2
	{ 0.55f, 0.55f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.55f, 0.55f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.55f, 0.f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,0.f }, // 4
	{ 0.30f, 0.f, 0,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,0.f } // 1
};

namespace DX = DirectX;
DX::XMVECTOR cornelBox[] = {
	//floor
	DX::XMVectorSet(552.8f, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//ceiling
	DX::XMVectorSet(552.8f, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//back-wall
	DX::XMVectorSet(549.6f, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//right-wall
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(0,1.0f,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(0,1.0f,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 0,1),DX::XMVectorSet(0,1.0f,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(0,1.0f,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//left-wall
	DX::XMVectorSet(552.8f, 0, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-1
	DX::XMVectorSet(130.0f, 165.0f,  65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f,  165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-2
	DX::XMVectorSet(290.0f,   0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-3
	DX::XMVectorSet(130.0f, 0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-4
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f,  0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-5
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),

	//rectangle - 2-1
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-2
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-3
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-4
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-5
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
};

uint32_t g_indecies1[] = {
	0,1,2,2,3,0,
	4,5,6,6,7,4,
	8,9,10,10,11,8,
	12,13,14,14,15,12,
	16,17,18,18,19,16,
	20,21,22,22,23,20,
	24,25,26,26,27,24,
	28,29,30,30,31,28,
	32,33,34,34,35,32,
	36,37,38,38,39,36,
	40,41,42,42,43,40,
	44,45,46,46,47,44,
	48,49,50,50,51,48,
	52,53,54,54,55,52,
	56,57,58,58,59,56,
};

char def_shader[] =
"#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0)\"\n \
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
[RootSignature(rootSig)]\
PSInput VSMain(float4 position : POSITION,float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	result.color = color;\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return float4(input.color.xyz,1);\
}\
";

char tex_shader[] ="\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),CBV(b1),DescriptorTable(SRV(t0)),StaticSampler(s0) \"\n\
cbuffer cb1 : register(b0){\
float s1;\
};\
cbuffer cb2 : register(b1){\
float s2;\
};\
Texture2D<float4> tex_ : register(t0);\
SamplerState samp_ : register(s0);\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
	float2	uv : TEXCOORD0;\
};\
[RootSignature(rootSig)]\
PSInput VSMain(float3 position : POSITION,float4 color : COLOR,float2 uv : TEXCOORD0){\
	PSInput	result;\
	result.position = float4(position,1)+float4(s1,s2,0,0);\
	result.color = color;\
	result.uv = uv;\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET{\
	return input.color*tex_.Sample(samp_, input.uv) + float4(0,0,0,1);\
}\
";

HRESULT create_pipeline(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), L"VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), L"PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 2);
	pipe.view = {
		.Width = static_cast<FLOAT>(ino::d3d::screen_width),
		.Height = static_cast<FLOAT>(ino::d3d::screen_height),
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	};
	pipe.scissor = {
		.right = ino::d3d::screen_width,
		.bottom = ino::d3d::screen_height
	};

	return S_OK;
}

HRESULT create_pipeline_textured(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_shader, sizeof(tex_shader), L"VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_shader, sizeof(tex_shader), L"PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 3 + sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_FILTER filter[] = { D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR };
	D3D12_TEXTURE_ADDRESS_MODE warp[] = { D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
	pipe.CreateSampler(1, filter, warp);

	pipe.Create(elementDescs, 3);

	pipe.view = {
		.Width = static_cast<FLOAT>(ino::d3d::screen_width),
		.Height = static_cast<FLOAT>(ino::d3d::screen_height),
	};
	pipe.scissor = {
		.right = ino::d3d::screen_width,
		.bottom = ino::d3d::screen_height
	};

	return S_OK;
}

ino::d3d::texture loadTexture(std::istream& istream)
{
	BITMAPFILEHEADER header;
	BITMAPINFOHEADER info;
	istream.read((char*)&header, sizeof(header));
	istream.read((char*)&info, sizeof(info));

	int width = info.biWidth;
	int height = info.biHeight;
	byte* buffer = new byte[width*height*4];
	byte pixel[3];
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			istream.read((char*)pixel, 3);
			buffer[x * 4 + y * width * 4 + 0] = pixel[0];
			buffer[x * 4 + y * width * 4 + 1] = pixel[1];
			buffer[x * 4 + y * width * 4 + 2] = pixel[2];
			buffer[x * 4 + y * width * 4 + 3] = 0xff;
		}
	}
	ino::d3d::texture ret;
	ret.Create(width,height);
	ret.Map(buffer, width, height);
	delete[] buffer;
	return ret;
}

ino::d3d::texture createGradationTexture()
{
	byte* buffer = new byte[16*16*4];
	for(int i = 0;i < 16;++i)
		for (int j = 0; j < 16-1; ++j)
		{
			buffer[i * 4 + j * 16 * 4 + 0] = j*255/16;
			buffer[i * 4 + j * 16 * 4 + 1] = j*255/16;
			buffer[i * 4 + j * 16 * 4 + 2] = j*255/16 - j*125/16;
			buffer[i * 4 + j * 16 * 4 + 3] = 0xff;
		}
	ino::d3d::texture ret;
	ret.Create(16, 16);
	ret.Map(buffer, 16, 16);
	delete[] buffer;
	return ret;
}

#define resToStream(idr_,type) \
HRSRC resInfo = FindResource(0, MAKEINTRESOURCE(idr_),type);\
HGLOBAL resData = LoadResource(0, resInfo);\
LPVOID pvResData = LockResource(resData);\
\
DWORD size = SizeofResource(0, resInfo);\
struct membuf : std::streambuf {\
membuf(char* base, std::ptrdiff_t n) {\
	this->setg(base, base, base + n);\
}\
};\
membuf sbuf((char*)pvResData, size);\
std::istream resStream = std::istream(&sbuf)\

HGLOBAL Music()
{
	HRSRC resInfo = FindResource(0, MAKEINTRESOURCE(IDR_WAVE1), L"WAVE");
	HGLOBAL resData = LoadResource(0, resInfo);
	LPVOID pvResData = LockResource(resData);

	//mciSendCommand
	sndPlaySound((LPCWSTR)pvResData, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);

	return resData;
}

DirectX::XMMATRIX GenModelMatrix(const DirectX::XMVECTOR& pos, const DirectX::XMVECTOR& rot, const DirectX::XMVECTOR& sc)
{
	namespace DX = DirectX;
	const DX::XMMATRIX translate = DX::XMMatrixTranslation(DX::XMVectorGetX(pos), DX::XMVectorGetY(pos), DX::XMVectorGetZ(pos));
	const DX::XMMATRIX rotX = DX::XMMatrixRotationAxis(DX::XMVectorSet(1, 0, 0, 0), DX::XMVectorGetX(rot));
	const DX::XMMATRIX rotY = DX::XMMatrixRotationAxis(DX::XMVectorSet(0, 1, 0, 0), DX::XMVectorGetY(rot));
	const DX::XMMATRIX rotZ = DX::XMMatrixRotationAxis(DX::XMVectorSet(0, 0, 1, 0), DX::XMVectorGetZ(rot));
	const DX::XMMATRIX scale = DX::XMMatrixScaling(DX::XMVectorGetX(sc), DX::XMVectorGetY(sc), DX::XMVectorGetZ(sc));
	return
		DX::XMMatrixMultiply(
			DX::XMMatrixMultiply(
				DX::XMMatrixMultiply(
					DX::XMMatrixMultiply(rotY, rotX),
					rotZ
				),
				scale
			),
			translate
		);
}

static const int mesh_lower_case = 4;
static const int mesh_upper_case = mesh_lower_case + 26;
static const int mesh_num_case = mesh_upper_case + 26;

void drawText(std::vector<ino::d3d::StaticMesh>& meshes,ino::d3d::cbo<DX::XMFLOAT4X4> cbo[], ID3D12GraphicsCommandList* cmd,std::wstring const & str,DirectX::XMVECTOR pos,float scale,DirectX::XMMATRIX const& view, DirectX::XMMATRIX const& proj)
{
	namespace DX = DirectX;
	DirectX::XMFLOAT4X4 Mat;
	for (int i = 0; i < str.size(); ++i)
	{
		wchar_t c = str[i];
		if (c == L' ')
		{
			pos = DX::XMVectorAdd(pos,DX::XMVectorSet(scale+0.2,0,0,0));
			continue;
		}
		auto model = GenModelMatrix(pos,DX::XMVectorSet(0,0,0,0),DX::XMVectorSet(scale, scale, scale, 0));
		DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * proj));
		cbo[i].Set(cmd, Mat, 0);
		if(c >= L'a' && c <= L'z')
			meshes[mesh_lower_case + c - L'a' ].Draw(cmd);
		else if (c >= L'A' && c <= L'Z')
			meshes[mesh_upper_case + c - L'A'].Draw(cmd);
		else if (c >= L'0' && c <= L'9')
			meshes[mesh_num_case + c - L'0'].Draw(cmd);
		pos = DX::XMVectorAdd(pos, DX::XMVectorSet(scale + 0.2, 0, 0, 0));
	}
}

struct Plot {
	float Time;

	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR rot = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR scale = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
};

struct DrawObject {
	UINT shapeID = 0;
	UINT plotCount = 0;
	Plot plots[64] = {};

	UINT currentPlot = 0;
	float plotTimeEnd = 0;
	ino::d3d::cbo<DirectX::XMFLOAT4X4> cbo;
};
#include "export.txt"

#ifndef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif
	DWORD wnd_flg = WS_POPUP | WS_VISIBLE
#ifndef _DEBUG
		| WS_MAXIMIZE
#endif
		;

	HWND hwnd = CreateWindow(L"edit", 0, wnd_flg, 0, 0, ino::d3d::screen_width, ino::d3d::screen_height, 0, 0, 0, 0);
	printf("init=%d\n", ino::d3d::init(hwnd, false, ino::d3d::screen_width, ino::d3d::screen_height));
	//std::cout << ino::d3d::CheckDXRSupport(ino::d3d::device);
	//[[maybe_unused]] int a;

	//setup pipeline
	ino::d3d::cbo<float> timeCbo;
	ino::d3d::pipeline loadingPipe;
	create_pipeline_loading(loadingPipe);

	BOOL useDXR = FALSE;
	if (!ino::d3d::CheckDXRSupport(ino::d3d::device))
	{
		MessageBox(0, L"CheckDXRSupport = false", L"", MB_OK);
		return 0;
	}

	//resource
	timeCbo.Create();

	ino::d3d::vbo* vbo = new ino::d3d::vbo();
	vbo->Create(g_Vertices, 9 * sizeof(float), sizeof(g_Vertices));

	std::vector<ID3D12GraphicsCommandList*> cmds;
	const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 0.0f };
	auto draw_Loading = [hwnd,&cmds,&loadingPipe,&clearColor,&vbo,&timeCbo](float t) {
		MSG msg;
		if (PeekMessage(&msg,hwnd, 0, 0, 0));
		ino::d3d::begin();
		cmds.push_back(loadingPipe.Begin());
		loadingPipe.Clear(clearColor);
		timeCbo.Set(cmds.back(), t, 0);
		vbo->Draw(cmds.back());
		loadingPipe.End();

		ino::d3d::excute(&cmds[0], cmds.size());
		ino::d3d::wait();
		ino::d3d::end();
		Sleep(16);
		cmds.clear();
	};

	draw_Loading(0);//Offscreen Texture
	ino::d3d::renderTexture renderOffscreen[3];
	ino::d3d::renderTexture dxrOffscreen[3];
	{
		for (int i = 0; i < 3; ++i)
			renderOffscreen[i].Create(ino::d3d::screen_width, ino::d3d::screen_height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		for (int i = 0; i < 3; ++i)
			dxrOffscreen[i].Create(ino::d3d::screen_width, ino::d3d::screen_height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	}
	draw_Loading(0.1);//Mesh Create
	
	std::vector<ino::d3d::StaticMesh> meshes;
	
	ino::d3d::StaticMesh cornel_box;
	ino::d3d::vbo* vboCornelBox = new ino::d3d::vbo();
	ino::d3d::ibo* iboCornelBox = new ino::d3d::ibo();
	vboCornelBox->Create(cornelBox, 12 * sizeof(float), sizeof(cornelBox));
	iboCornelBox->Create(g_indecies1, sizeof(g_indecies1));
	cornel_box.vbo = *vboCornelBox;
	cornel_box.ibo = *iboCornelBox;

	meshes.push_back(ino::shape::CreateQuad());
	{
		resToStream(IDR_MODEL1, L"MODEL");
		meshes.push_back(ino::gfx::obj::load_obj(resStream));
	}
	meshes.push_back(ino::shape::CreateCube());
	meshes.push_back(ino::shape::CreateSphere(DirectX::XMVectorSet(1, 1, 1, 1), DirectX::XMVectorSet(1, 0, 0, 1)));
	for(wchar_t w = L'a';w <= L'z';++w)
		meshes.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));
	for(wchar_t w = L'A'; w <= L'Z'; ++w)
		meshes.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));
	for (wchar_t w = L'0'; w <= L'9'; ++w)
		meshes.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));
	draw_Loading(0.2);

	std::chrono::high_resolution_clock c;
	std::chrono::high_resolution_clock::time_point time_o = c.now();
	float rot = 0.0f;

	//texture load
	ino::d3d::texture meshTexture;
	{
		resToStream(IDR_TEXTURE1, L"TEXTURE");
		meshTexture = loadTexture(resStream);
	}
	ino::d3d::texture gradTex = createGradationTexture();

	draw_Loading(0.7);//DxrResources
	//Init DXR
	ino::d3d::begin();
	ino::d3d::dxr::InitDXRDevice();
	std::vector<ino::d3d::dxr::Blas> blases;
	blases.push_back(ino::d3d::dxr::Blas());
	ino::d3d::dxr::Tlas tlas;
	blases[0].Build(cornel_box);
	static const int mesh_start = 1;
	for (auto& m : meshes)
	{
		blases.push_back(ino::d3d::dxr::Blas());
		blases.back().Build(m);
	}

	ino::d3d::dxr::DxrPipeline dxrPipe;
	dxrPipe.Create();
	ino::d3d::end();

	draw_Loading(0.9);//RasterPipeline
	ino::d3d::pipeline pipe[2];
	create_pipeline(pipe[0]);
	create_pipeline_textured(pipe[1]);
	ino::d3d::pipeline offscreenPipe[2];
	ino::d3d::pipeline offscreenTextPipe[2];
	create_pipeline_tex_gen1(offscreenPipe[0]);
	create_pipeline(offscreenTextPipe[0]);
	create_pipeline_tex_gen2(offscreenPipe[1]);
	create_pipeline(offscreenTextPipe[1]);
	ino::d3d::pipeline postPipe;
	create_pipeline_postProcess(postPipe);
	ino::d3d::pipeline denoisePipe[5];
	for(int i = 0;i<5;++i)
		create_pipeline_denoise(denoisePipe[i]);

	draw_Loading(1.0);//ConstantBufferObjects
	ino::d3d::cbo<DX::XMFLOAT4X4> mvpCBO[256];
	ino::d3d::cbo<float> fCBO[2];
	ino::d3d::cbo<DenoiseCBO> denoiseCBO;
	{
		for(auto &cbo : mvpCBO)
			cbo.Create();
		for (auto& val : fCBO)
			val.Create();
		denoiseCBO.Create();
	}

	Sleep(500);

	draw_Loading(1);
	//playSound
	auto musicHandle = Music();
	ShowCursor(false);
	srand(0);
	cmds.reserve(256);
	int dxrOffscreenCount = 0;

	PlotCamera cam;
	int camPlotsIndex = 0;
	do {
		cmds.clear();
		MSG msg;
		if (PeekMessage(&msg, hwnd, 0, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		auto model = DX::XMMatrixIdentity();
		rot = local_time * 0.3f;
		if (camPlotsIndex < _countof(cameraPlot) - 1 && local_time > cameraPlot[camPlotsIndex + 1].time)
			camPlotsIndex += 1;

		if (cameraPlot[_countof(cameraPlot) - 1].time >= local_time)
		{
			PlotCamera c1 = cameraPlot[camPlotsIndex];
			PlotCamera c2 = cameraPlot[camPlotsIndex + 1];

			const float w = c2.time - c1.time;
			const float ab = std::min(1.f, (c2.time - local_time) / w);
			cam.pos = DX::XMVectorAdd(DX::XMVectorScale(c1.pos, ab), DX::XMVectorScale(c2.pos, 1.f - ab));
			cam.target = DX::XMVectorAdd(DX::XMVectorScale(DX::XMVectorAdd(c1.pos, c1.target), ab), DX::XMVectorScale(DX::XMVectorAdd(c2.pos, c2.target), 1.f - ab));
			cam.up = DX::XMVectorAdd(DX::XMVectorScale(c1.up, ab), DX::XMVectorScale(c2.up, 1.f - ab));
		}
		else
		{
			cam = cameraPlot[_countof(cameraPlot) - 1];
		}
		auto view = DX::XMMatrixLookAtLH(cam.pos, cam.target, cam.up);
		auto projection = DX::XMMatrixPerspectiveFovLH(DX::XMConvertToRadians(60), ino::d3d::screen_width / (float)ino::d3d::screen_height, 0.01f, 100.f);
		//auto invView = DX::XMMatrixInverse(nullptr, view);
		auto invViewProj = DX::XMMatrixInverse(nullptr, XMMatrixTranspose(model * view * projection));

		DX::XMFLOAT4X4 Mat;
		DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));

		local_time += std::chrono::duration_cast<std::chrono::milliseconds>(c.now() - time_o).count() / 1000.f;
		time_o = c.now();
		ino::d3d::begin();

		//create tlas
		{
			tlas.ClearInstance();
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[1], {
					GenModelMatrix(DX::XMVectorSet(-8.3,6.43,-1.35,1),DX::XMVectorSet(0,3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[0], {
					GenModelMatrix(DX::XMVectorSet(-8.3,6.43,-9.15,1),DX::XMVectorSet(0,3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[2], {
					GenModelMatrix(DX::XMVectorSet(-8.3,6.43,-17.55,1),DX::XMVectorSet(0,3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[2], {
					GenModelMatrix(DX::XMVectorSet(8.3,6.43,-1.35,1),DX::XMVectorSet(0,-3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[0], {
					GenModelMatrix(DX::XMVectorSet(8.3,6.43,-9.15,1),DX::XMVectorSet(0,-3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[1], {
					GenModelMatrix(DX::XMVectorSet(8.3,6.43,-17.55,1),DX::XMVectorSet(0,-3.141592 / 2.0,0,1),DX::XMVectorSet(1,1.95,3.4,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[1], {
					GenModelMatrix(DX::XMVectorSet(3.8,6.43,7.25,1),DX::XMVectorSet(0,3.141592,0,1),DX::XMVectorSet(3.4,1.95,1,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start], &renderOffscreen[0], {
					GenModelMatrix(DX::XMVectorSet(-3.8,6.43,7.25,1),DX::XMVectorSet(0,3.141592,0,1),DX::XMVectorSet(3.4,1.95,1,0)),
					1.5f, 1.f , tlas.GetBlasCount()
				}
			);
			tlas.AddInstance(&blases[mesh_start + 1], &meshTexture, { DirectX::XMMatrixScaling(1, 1, 1),0,0.2,tlas.GetBlasCount() });
			tlas.AddInstance(&blases[mesh_start + 2], { DirectX::XMMatrixScaling(6, 5, 12) * DirectX::XMMatrixTranslation(0,16,1),3.0,0.1 });
			if (local_time > 86.f)
			{
				auto mat = GenModelMatrix(DX::XMVectorSet(-9+(local_time - 86.f),4.5+abs(sin(local_time-86.f)),8.7,0), DX::XMVectorSet(0,local_time,0,0), DX::XMVectorSet(0.5,0.5,0.5,1));
				tlas.AddInstance(&blases[mesh_start + 3], { mat,0,1 });
			}

			for (auto& val : drawObj)
			{
				if (val.plotCount < 2)
					continue;
				if (local_time > val.plotTimeEnd)
					continue;
				if (val.plots[0].Time > local_time)
					continue;

				int currentPlot = 0;
				for (int i = 0; i < val.plotCount - 1; ++i)
				{
					if (val.plots[i].Time <= local_time && val.plots[i + 1].Time >= local_time)
						currentPlot = i;
				}
				val.plots[currentPlot].mat = GenModelMatrix(val.plots[currentPlot].pos, val.plots[currentPlot].rot, DX::XMVectorSet(1,1,1,1));
				val.plots[currentPlot+1].mat = GenModelMatrix(val.plots[currentPlot + 1].pos, val.plots[currentPlot + 1].rot, DX::XMVectorSet(1, 1, 1, 1));

				const float w = val.plots[currentPlot + 1].Time - val.plots[currentPlot].Time;
				const float ab = std::min(1.f, (val.plots[currentPlot + 1].Time - local_time) / w);
				model = val.plots[currentPlot].mat * (ab)+val.plots[currentPlot + 1].mat * (1.f - ab);
				tlas.AddInstance(&blases[val.shapeID - 1],&gradTex,{ model,0,1 });
			}
			tlas.Build();
		}

		ino::d3d::dxr::SceneConstant constants;
		constants.cameraPos = cam.pos;// DX::XMVectorSet(0, 0, -1, 1);
		constants.invViewProj = invViewProj;
		constants.time = local_time;
		dxrPipe.SetConstantBuffer(&constants, sizeof(ino::d3d::dxr::SceneConstant));

		static const FLOAT clearColor2[] = { 1.f, 1.f, 0.f, 1.0f };
		//offscreen
		for (int i = 0; i < 2; ++i)
		{
			cmds.push_back(offscreenPipe[i].Begin(renderOffscreen[i]));
			offscreenPipe[i].Clear(renderOffscreen[i], clearColor2);
			timeCbo.Set(cmds.back(), local_time, 0);
			vbo->Draw(cmds.back());
			offscreenPipe[i].End();
		}

		//DrawText
		{
			float fade = std::min(1.f,std::max(0.f,local_time*0.5f - 2.5f));
			float scale = 0.8 * fade;
			cmds.push_back(offscreenTextPipe[0].Begin(renderOffscreen[0]));
			offscreenTextPipe[0].ClearDepth(renderOffscreen[0]);
			auto view2 = DX::XMMatrixLookAtLH(DX::XMVectorSet(0, 0,-1, 0), DX::XMVectorSet(0, 0, 0, 0), DX::XMVectorSet(0, 1, 0, 0));
			drawText(meshes, mvpCBO + 2, cmds.back(), L"PATH", DX::XMVectorSet(-1.1, 1, 5, 0), scale, view2, projection);
			drawText(meshes, mvpCBO + 6, cmds.back(), L"Tonight", DX::XMVectorSet(-1.5, -0.4f, 5, 0), scale*0.5f, view2, projection);

			offscreenTextPipe[0].End();
		}

		{
			float t = local_time*4;
			t -= 60.f * (((int)t)/60);
			float scroll = -t + 20.f;
			cmds.push_back(offscreenTextPipe[1].Begin(renderOffscreen[1]));
			offscreenTextPipe[1].ClearDepth(renderOffscreen[1]);
			auto view2 = DX::XMMatrixLookAtLH(DX::XMVectorSet(0, 0, -1, 0), DX::XMVectorSet(0, 0, 0, 0), DX::XMVectorSet(0, 1, 0, 0));
			drawText(meshes, mvpCBO + 13, cmds.back(), L"Wellcome back TokyoDemoFest", DX::XMVectorSet(scroll, -1, 5, 0), 1.f, view2, projection);

			offscreenTextPipe[1].End();
		}

		cmds.push_back(pipe[0].Begin(renderOffscreen[2]));
		pipe[0].Clear(renderOffscreen[2], clearColor2);
		auto view2 = DX::XMMatrixLookAtLH(DX::XMVectorSet(cos(rot) * 16, 8, sin(rot) * 16, 0), DX::XMVectorSet(0, 1.5, 0, 0), DX::XMVectorSet(0, 1, 0, 0));
		model = GenModelMatrix(DX::XMVectorSet(0,0, 0, 0), DX::XMVectorSet(0, 0, 0, 0), DX::XMVectorSet(0.01f, 0.01f, 0.01f, 0));
		DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model* view2* projection));
		mvpCBO[0].Set(cmds.back(), Mat, 0);
		vboCornelBox->Draw(cmds.back(), *iboCornelBox);
		{
			float t = local_time * 4;
			t -= 60.f * (((int)t) / 60);
			float scroll = -t + 20.f;
			auto view3 = DX::XMMatrixLookAtLH(DX::XMVectorSet(0, 0, -1, 0), DX::XMVectorSet(0, 0, 0, 0), DX::XMVectorSet(0, 1, 0, 0));
			drawText(meshes, mvpCBO + 40, cmds.back(), L"next is greeting time", DX::XMVectorSet(scroll, 2.5, 5, 0), 0.5f+abs(sin(local_time*4)*0.3), view3, projection);
		}
		pipe[0].End();

		cmds.push_back(pipe[1].Begin());
		pipe[1].Clear(clearColor);
		fCBO[0].Set(cmds.back(),sinf(local_time),0);
		fCBO[1].Set(cmds.back(),cosf(local_time),1);
		renderOffscreen[0].Set(cmds.back(),2);
		//DrawPrimitive;
		renderOffscreen[1].Set(cmds.back(),2);
		vbo->Draw(cmds.back());
		pipe[1].End();

		//Raytrace!
		{
			cmds.push_back(dxrPipe.Begin().Get());
			dxrPipe.CreateShaderTable(tlas);
			dxrPipe.Dispatch(tlas);
			for (int i = 0; i < 3; ++i)
			{
				dxrOffscreen[i].RenderTarget(cmds.back());
				dxrPipe.CopyToScreen(dxrOffscreen[i], i);
			}
			dxrPipe.End();
		}

		for (int i = 0; i < 5; ++i)//denoise scene
		{
			cmds.push_back(denoisePipe[i].Begin(renderOffscreen[i%2]));
			denoisePipe[i].Clear(renderOffscreen[i%2], clearColor2);
			DenoiseCBO cb = {};
			cb.width = denoisePipe[i].view.Width;
			cb.height = denoisePipe[i].view.Height;
			cb.c_phi = 0.0225f * pow(2,i);
			cb.n_phi = 0.0225f * pow(2,i);
			cb.p_phi = 0.0225f * pow(2,i);
			cb.stepwidth = i*2;
			denoiseCBO.Set(cmds.back(), cb, 0);
			if (i == 0)
				dxrOffscreen[0].Set(cmds.back(), 1);
			else
				renderOffscreen[(i + 1) % 2].Set(cmds.back(), 1);
			dxrOffscreen[1].Set(cmds.back(), 2);
			dxrOffscreen[2].Set(cmds.back(), 3);
			vbo->Draw(cmds.back());
			denoisePipe[i].End();
		}

		cmds.push_back(postPipe.Begin());
		timeCbo.Set(cmds.back(), local_time, 0);
		renderOffscreen[1].Set(cmds.back(), 1);
		vbo->Draw(cmds.back());
		postPipe.End();

		ino::d3d::excute(&cmds[0], cmds.size());
		ino::d3d::wait();
		Sleep(4);
		ino::d3d::end();
		//dxrPipe.ClearShaderTable();
#ifdef _DEBUG
		std::cout << local_time << std::endl;
#endif
	} while (!GetAsyncKeyState(VK_ESCAPE) && local_time < 102.5f);//&& local_time < 144.83f);
	
	Sleep(500);
	delete vbo;
	delete vboCornelBox;
	delete iboCornelBox;

	UnlockResource(musicHandle);
	FreeResource(musicHandle);

	ino::d3d::release();
	ShowCursor(true);
	return 0;
}
