#include "../dx/dx.hpp"
#include "../dx/dx12pipeline.hpp"
#include "../dx/dx12resource.hpp"
#include "dx_dll.h"

#include "../gfx/shapes.hpp"

#include <DirectXMath.h>
#include <vector>

float t = 0.f;
bool plotMode = false;

struct Camera {
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0,0,-1.0,0);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(0,0,1,0);;
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);;
	float plotTime;
};
std::vector<Camera> camPlots;
static Camera cam;

struct DrawObject {
	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
	UINT shapeID;

	float plotTimeBegin;
	float plotTimeEnd;
};
typedef std::vector<DrawObject> ObjectPlots;
std::vector<ObjectPlots> objPlots;

std::vector<ino::d3d::vbo> vbos;

char def_shader[] =
"\
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
PSInput VSMain(float4 position : POSITION,float4 normal : NORMAL,float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	float3 norm = mul(float4(normal.xyz,0),mvp).xyz;\
	result.color = float4(color.xyz * dot(float3(0,0.5,0.5),norm),1);\
	return result;\
}\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color;\
}\
";

char tex_shader[] =
"\
Texture2D<float4> tex_ : register(t0);\
SamplerState samp_ : register(s0);\
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float2	uv : TEXCOORD0;\
};\
PSInput VSMain(float4 position : POSITION,float4 normal : NORMAL,float4 uv : TEXCOORD0){\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	result.uv = uv.xy;\
	return result;\
}\
float4 PSMain(PSInput input) : SV_TARGET{\
	return tex_.Sample(samp_, input.uv);\
}\
";

HRESULT create_pipeline(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 3, 1, true);
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_FILTER filter[] = { D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR };
	D3D12_TEXTURE_ADDRESS_MODE wrap[] = { D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
	pipe.CreateSampler(1, filter, wrap);

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

static ino::d3d::pipeline pipe[2];
static ino::d3d::cbo<DirectX::XMFLOAT4X4> mvpCBO;


DLL_EXPORT BOOL InitDxContext(HWND hwnd, UINT width, UINT height)
{
	BOOL ret = ino::d3d::init(hwnd, false, width, height) == S_OK;
	if (!ret) return ret;
	//setup pipeline
	create_pipeline(pipe[0]);
	create_pipeline_textured(pipe[1]);

	mvpCBO.Create();

	//load default shape
	vbos.push_back( ino::shape::CreateCube() );
	vbos.push_back( ino::shape::CreateCube() );

	return ret;
}

DLL_EXPORT BOOL ReleaseDxContext()
{
	try
	{
		ino::d3d::release();
	}
	catch (...)
	{
		std::wcout << L"release error" << std::endl;
		return FALSE;
	}

	return TRUE;
}

DLL_EXPORT BOOL DxContextFlush()
{
	namespace DX = DirectX;

	float rot = t;
	auto model = DX::XMMatrixIdentity();
	//DX::XMVector3Cross(1, 2);
	DX::XMMATRIX view = DX::XMMatrixLookAtLH(cam.pos,cam.target,cam.up);
	auto projection = DX::XMMatrixPerspectiveFovLH(DX::XMConvertToRadians(60), screen_width / (float)screen_height, 0.01f, 100.f);
	DX::XMFLOAT4X4 Mat;
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	
	ID3D12GraphicsCommandList* cmds[_countof(pipe)] = {};
	ino::d3d::begin();

	static const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 0.0f };
	cmds[0] = pipe[0].Begin();
	pipe[0].Clear(clearColor);
	mvpCBO.Set(cmds[0], Mat, 0);
	vbos[0].Draw(cmds[0]);
	pipe[0].End();

	cmds[1] = pipe[1].Begin();
	pipe[1].End();

	ino::d3d::excute(cmds, _countof(pipe));
	ino::d3d::wait();
	ino::d3d::end();
	return TRUE;
}

DLL_EXPORT void SetTime(UINT t_)
{
	t = t_/1000.f;
}
DLL_EXPORT void SetPlotMode(BOOL isPlot)
{
	plotMode = isPlot;
}

DLL_EXPORT void SetCurrentCamera(point p, point t, point u)
{
	namespace DX = DirectX;
	cam.pos = DX::XMVectorSet(p.x,p.y,p.z,p.w);
	cam.target = DX::XMVectorSet(t.x, t.y, t.z, t.w);
	cam.up = DX::XMVectorSet(u.x, u.y, u.z, u.w);
}
