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

static float g_Vertices[6][9] = {
	{ 0.10f, 0.f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,0.f }, // 1
	{ 0.1f, 0.25f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,1.f }, // 2
	{ 0.35f, 0.25f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.35f, 0.25f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.35f, 0.f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/1.f,0.f }, // 4
	{ 0.10f, 0.f, 0.5f,/**/ 1.0f, 1.0f, 1.0f,1.0f,/**/0.f,0.f } // 1
};

static float g_Vertices2[6][9] = {
	{ 0.0f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 0.0f,1.0f,/**/0.f,0.f }, // 1
	{ 0.f, 0.25f, 0.0f,/**/ 0.0f, 1.0f, 0.0f,1.0f,/**/0.f,1.f }, // 2
	{ 0.25f, 0.25f, 0.0f,/**/ 0.0f, 0.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.25f, 0.25f, 0.0f,/**/ 0.0f, 0.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 0.25f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 1.0f,1.0f,/**/1.f,0.f }, // 4
	{ 0.0f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 0.0f,1.0f,/**/0.f,0.f } // 1
};

namespace DX = DirectX;
DX::XMVECTOR cornelBox[] = {
	//floor
	DX::XMVectorSet(552.8f, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(552.8f, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//ceiling
	DX::XMVectorSet(552.8f, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(552.8f, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//back-wall
	DX::XMVectorSet(549.6f, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//right-wall
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 548.8f, 0,1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 548.8f, 0,1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(0,1.0f,0,1.0f),
	//left-wall
	DX::XMVectorSet(552.8f, 0, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(552.8f, 0, 0, 1),DX::XMVectorSet(1.0f,0,0,1.0f),
	//rectangle - 1-1
	DX::XMVectorSet(130.0f, 165.0f,  65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f,  165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f,  65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-2
	DX::XMVectorSet(290.0f,   0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f,   0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-3
	DX::XMVectorSet(130.0f, 0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-4
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f,  0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-5
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),

	//rectangle - 2-1
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-2
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-3
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-4
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-5
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
};

char def_shader[] =
"\
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
PSInput VSMain(float4 position : POSITION,float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = mul(float4(position.xyz*0.01f,1),mvp);\
	result.color = color;\
	return result;\
}\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color;\
}\
";

char tex_shader[] =
"\
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
PSInput VSMain(float3 position : POSITION,float4 color : COLOR,float2 uv : TEXCOORD0){\
	PSInput	result;\
	result.position = float4(position,1)+float4(s1,s2,0,0);\
	result.color = color;\
	result.uv = uv;\
	return result;\
}\
float4 PSMain(PSInput input) : SV_TARGET{\
	return input.color*tex_.Sample(samp_, input.uv);\
}\
";

HRESULT create_pipeline(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 2,1,true);
	pipe.view = {
		.Width = static_cast<FLOAT>(screen_width),
		.Height = static_cast<FLOAT>(screen_height),
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	};
	pipe.scissor = {
		.right = screen_width,
		.bottom = screen_height
	};

	return S_OK;
}

HRESULT create_pipeline_textured(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_shader, sizeof(tex_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_shader, sizeof(tex_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float)*3+sizeof(float)*4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_FILTER filter[] = { D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR };
	D3D12_TEXTURE_ADDRESS_MODE warp[] = { D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
	pipe.CreateSampler(1, filter, warp);

	pipe.Create(elementDescs, 3, 2, false);

	pipe.view = {
		.Width = static_cast<FLOAT>(screen_width),
		.Height = static_cast<FLOAT>(screen_height),
	};
	pipe.scissor = {
		.right = screen_width,
		.bottom = screen_height
	};

	return S_OK;
}

static BYTE texData[] = {
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,	0xff,0x9f,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0x4f,	0xff,0x8f,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0x3f,	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,
};

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

	HWND hwnd = CreateWindow(L"edit", 0, wnd_flg, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
	printf("init=%d\n", ino::d3d::init(hwnd, false, screen_width, screen_height));
	//std::cout << ino::d3d::CheckDXRSupport(ino::d3d::device);
	//[[maybe_unused]] int a;

	//setup pipeline
	ino::d3d::cbo<DX::XMFLOAT4X4> mvpCBO;
   	ino::d3d::cbo<float> fCBO[2];

	ino::d3d::pipeline pipe[2];
	create_pipeline(pipe[0]);
	create_pipeline_textured(pipe[1]);

	BOOL useDXR = FALSE;
	if (ino::d3d::CheckDXRSupport(ino::d3d::device))
	{
		useDXR = ino::d3d::InitDXRDevice();
	}

	//resource
	mvpCBO.Create();
	for (auto& val : fCBO)
		val.Create();

	ID3D12GraphicsCommandList* cmds[_countof(pipe)] = {};
	ino::d3d::vbo* vbo = new ino::d3d::vbo();
	vbo->Create(g_Vertices, 9 * sizeof(float), sizeof(g_Vertices));

	ino::d3d::vbo* vbo2 = new ino::d3d::vbo();
	vbo2->Create(g_Vertices2, 9 * sizeof(float), sizeof(g_Vertices2));

	ino::d3d::vbo* vboCornelBox = new ino::d3d::vbo();
	vboCornelBox->Create(cornelBox,8*sizeof(float),sizeof(cornelBox));

	ino::d3d::texture tex;
	tex.Create(4,4);
	tex.Map(texData,4,4, 4);
	DWORD time_o = ::timeGetTime();
	float rot = 0.0f;
	srand(0);
	do {
		MSG msg;
		if (PeekMessage(&msg, hwnd, 0, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		local_time = (::timeGetTime() - time_o) / 1000.f;

		std::mutex m;

		//std::thread t1([&cmds, &pipe,&vboCornelBox, &m,&mvpCBO]() {
			auto model = DX::XMMatrixIdentity();
			rot += 0.01f;
			auto view = DX::XMMatrixLookAtLH(DX::XMVectorSet(cos(rot)*13, 3, sin(rot)*13, 0), DX::XMVectorSet(0, 1.5, 0, 0),DX::XMVectorSet(0,1,0,0));
			auto projection = DX::XMMatrixPerspectiveFovLH(DX::XMConvertToRadians(60), screen_width / (float)screen_height, 0.01f, 100.f);
			//DX::XMMATRIX mvp = model * view * projection;
			DX::XMFLOAT4X4 Mat;
			DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
				const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 0.0f };
				m.lock();
				cmds[0] = pipe[0].Begin();
				mvpCBO.Set(cmds[0], Mat,0);
				pipe[0].Clear(clearColor);
				//DrawPrimitive;
				vboCornelBox->Draw(cmds[0]);
				pipe[0].End(); 
				m.unlock();
		//	}
		//);

		//const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		m.lock();
		cmds[1] = pipe[1].Begin();
		fCBO[0].Set(cmds[1],sinf(local_time),0);
		fCBO[1].Set(cmds[1],cosf(local_time),1);
		tex.Set(cmds[1],2);
		//DrawPrimitive;
		vbo2->Draw(cmds[1]);
		vbo->Draw(cmds[1]);
		pipe[1].End();
		m.unlock();

		//t1.join();

		ino::d3d::excute(cmds, _countof(pipe));
		ino::d3d::wait();
		ino::d3d::flush();
	} while (!GetAsyncKeyState(VK_ESCAPE));//&& local_time < 144.83f);
	
	Sleep(500);
	delete vbo;
	delete vbo2;
	delete vboCornelBox;

	
	ino::d3d::release();
	return 0;
}
