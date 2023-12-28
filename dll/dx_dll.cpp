#define NOMINMAX

#include "../dx/dx.hpp"
#include "../dx/dx12pipeline.hpp"
#include "../dx/dx12resource.hpp"
#include "dx_dll.h"

#include "../gfx/shapes.hpp"
#include "../gfx/font.hpp"
#include "../gfx/obj_loader.hpp"

#include <DirectXMath.h>
#include <vector>

#include <iostream>
#include <fstream>

float t = 0.f;
UINT plotMode = 0;

static const float frameTime = (1000.f);

inline point XMVec2Point(DirectX::XMVECTOR v)
{
	return point{ DirectX::XMVectorGetX(v),DirectX::XMVectorGetY(v),DirectX::XMVectorGetZ(v),DirectX::XMVectorGetW(v) };
}

inline DirectX::XMVECTOR Point2XmVec(point const& v)
{
	return DirectX::XMVectorSet(v.x,v.y,v.z,v.w);
}


struct Camera {
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(0,0,-1.0,0);
	DirectX::XMVECTOR target = DirectX::XMVectorSet(0,0,1,0);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);
	float plotTime;
};
std::vector<Camera> camPlots;
static int camPlotsIndex = 0;
static Camera cam;

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

DirectX::XMMATRIX GenModelMatrix(const point& pos, const point& rot, const point& sc)
{
	namespace DX = DirectX;
	const DX::XMMATRIX translate = DX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	const DX::XMMATRIX rotX = DX::XMMatrixRotationAxis(DX::XMVectorSet(1, 0, 0, 0), rot.x);
	const DX::XMMATRIX rotY = DX::XMMatrixRotationAxis(DX::XMVectorSet(0, 1, 0, 0), rot.y);
	const DX::XMMATRIX rotZ = DX::XMMatrixRotationAxis(DX::XMVectorSet(0, 0, 1, 0), rot.z);
	const DX::XMMATRIX scale = DX::XMMatrixScaling(sc.x, sc.y, sc.z);
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
std::vector<DrawObject> objPlots;
static DrawObject obj = {};

std::vector<ino::d3d::StaticMesh> mesh;

void GenCameraPlotIndecis()
{

}

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
PSInput VSMain(float4 position : POSITION,float4 normal : NORMAL,float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	float3 norm = mul(float4(normal.xyz,0),mvp).xyz;\
	result.color = float4(color.xyz * dot(float3(0,0.5,0.5),norm),1);\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color;\
}\
";

char selected_shader[] =
"#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0)\"\n \
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
[RootSignature(rootSig)]\
PSInput VSMain(float4 position : POSITION,float4 normal : NORMAL,float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	float3 norm = mul(float4(normal.xyz,0),mvp).xyz;\
	result.color = float4(color.xyz * dot(float3(0,0.5,0.5),norm),1);\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color*float4(1,0.4,0.4,0.4);\
}\
";

char tex_shader[] =
"\
#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),DescriptorTable(SRV(t0)),StaticSampler(s0) \"\n \
cbuffer cb1 : register(b0){\
float4x4 mvp;\
};\
Texture2D<float4> tex_ : register(t0);\
SamplerState samp_ : register(s0);\
struct PSInput {\
	float4	position : SV_POSITION;\
	float2	uv : TEXCOORD0;\
};\
[RootSignature(rootSig)]\
PSInput VSMain(float4 position : POSITION,float4 uv : TEXCOORD0,float4 color : COLOR){\
	PSInput	result;\
	result.position = mul(float4(position.xyz,1),mvp);\
	result.uv = uv.xy;\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET{\
	return tex_.Sample(samp_, input.uv);\
}\
";

HRESULT create_pipeline(ino::d3d::pipeline& pipe)
{
	{
		//shader_resource vs_resorce = resorce_ptr(IDR_DEFAULT_VS);
		//shader_resource ps_resorce = resorce_ptr(IDR_DEFAULT_PS);
		//pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, vs_resorce.bytecode.get(), vs_resorce.size);
		//pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, ps_resorce.bytecode.get(), ps_resorce.size);
	}

	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 3);
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

HRESULT create_pipeline_selected(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, selected_shader, sizeof(selected_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, selected_shader, sizeof(selected_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 3);
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
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_shader, sizeof(tex_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_shader, sizeof(tex_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	D3D12_FILTER filter[] = { D3D12_FILTER::D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR };
	D3D12_TEXTURE_ADDRESS_MODE wrap[] = { D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP };
	pipe.CreateSampler(1, filter, wrap);

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

static ino::d3d::pipeline pipe[3];
static ino::d3d::cbo<DirectX::XMFLOAT4X4> mvpCBO;

static ino::d3d::cbo<DirectX::XMFLOAT4X4> u_cbo;
static ino::d3d::cbo<DirectX::XMFLOAT4X4> n_cbo;
static ino::d3d::cbo<DirectX::XMFLOAT4X4> ti_cbo;
static ino::d3d::cbo<DirectX::XMFLOAT4X4> offscreen_cbo;

DLL_EXPORT BOOL InitDxContext(HWND hwnd, UINT width, UINT height)
{
	BOOL ret = ino::d3d::init(hwnd, false, width, height) == S_OK;
	if (!ret) return ret;
	//setup pipeline
	create_pipeline(pipe[0]);
	create_pipeline_textured(pipe[1]);
	create_pipeline_selected(pipe[2]);

	camPlots.reserve(2048);
	objPlots.reserve(2048);

	obj.cbo.Create();
	mvpCBO.Create();

	u_cbo.Create();
	n_cbo.Create();
	ti_cbo.Create();
	offscreen_cbo.Create();
	//load default shape
	mesh.push_back(ino::shape::CreateCube() );
	mesh.push_back(ino::shape::CreateQuad());
	mesh.push_back(ino::shape::CreateSphere(DirectX::XMVectorSet(1, 1, 1, 1), DirectX::XMVectorSet(1, 0, 0, 1)));
	std::ifstream ifs("resource/shop.model", std::ios::in);
	if (ifs)
		mesh.push_back(ino::gfx::obj::load_obj(ifs));
	static const int mesh_lower_case = 3;
	for (wchar_t w = L'a'; w <= L'z'; ++w)
		mesh.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));
	static const int mesh_upper_case = 3 + 26;
	for (wchar_t w = L'A'; w <= L'Z'; ++w)
		mesh.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));
	static const int mesh_num_case = 3 + 26 + 26;
	for (wchar_t w = L'0'; w <= L'9'; ++w)
		mesh.push_back(ino::shape::CreateCharMesh(w, L"Ariel"));

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
	if (plotMode)
	{
		if (camPlots.size() > camPlotsIndex + 1)
		{
			Camera c1 = camPlots[camPlotsIndex];
			Camera c2 = camPlots[camPlotsIndex + 1];

			const float w = c2.plotTime - c1.plotTime;
			const float ab = std::min(1.f, (c2.plotTime - t) / w);
			cam.pos = DX::XMVectorAdd(DX::XMVectorScale(c1.pos, ab), DX::XMVectorScale(c2.pos, 1.f - ab));
			cam.target = DX::XMVectorAdd(DX::XMVectorScale(DX::XMVectorAdd(c1.pos,c1.target), ab), DX::XMVectorScale(DX::XMVectorAdd(c2.pos, c2.target), 1.f - ab));
			cam.up = DX::XMVectorAdd(DX::XMVectorScale(c1.up, ab), DX::XMVectorScale(c2.up, 1.f - ab));
		}
	}
	DX::XMMATRIX view = DX::XMMatrixLookAtLH(cam.pos,cam.target,cam.up);
	auto projection = DX::XMMatrixPerspectiveFovLH(DX::XMConvertToRadians(60), ino::d3d::screen_width / (float)ino::d3d::screen_height, 0.01f, 100.f);
	DX::XMFLOAT4X4 Mat;
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	
	ID3D12GraphicsCommandList* cmds[_countof(pipe)] = {};
	ino::d3d::begin();
	static const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	static const FLOAT clearColor2[] = { 1.f, 0.f, 0.f, 1.0f };
	//offscreen
	//cmds[0] = pipe[0].Begin(ino::d3d::renderOffscreen);
	//pipe[0].Clear(clearColor2);
	//pipe[0].End();

	cmds[0] = pipe[0].Begin();
	pipe[0].Clear(clearColor);
	for (auto& val : objPlots)
	{
		if (val.plotCount < 2)
			continue;
		if (t > val.plotTimeEnd)
			continue;

		int currentPlot = 0;
		for (int i = 0; i < val.plotCount - 1; ++i)
		{
			if(val.plots[i].Time <= t && val.plots[i + 1].Time >= t)
				currentPlot = i;
		}

		const float w = val.plots[currentPlot +1].Time - val.plots[currentPlot].Time;
		const float ab = std::min(1.f, (val.plots[currentPlot+1].Time - t) / w);
		model = val.plots[currentPlot].mat *(ab) +val.plots[currentPlot+1].mat *(1.f - ab);
		DX::XMStoreFloat4x4(&Mat,XMMatrixTranspose(model * view * projection));
		val.cbo.Set(cmds[0],Mat,0);
		mesh[val.shapeID].Draw(cmds[0]);
	}
	//model = GenModelMatrix(point(-5, 10, 0, 0), point(0, 0, 0, 0), point(0.025, 0.025, 1, 1));
	//DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	//u_cbo.Set(cmds[0], Mat, 0);
	//mesh[2].Draw(cmds[0]);

	//model = GenModelMatrix(point(0, 10, 0, 0), point(0, 0, 0, 0), point(0.025, 0.025, 1, 1));
	//DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	//n_cbo.Set(cmds[0], Mat, 0);
	//mesh[3].Draw(cmds[0]);

	//model = GenModelMatrix(point(5,10,0,0), point(0,0,0,0), point(0.025,0.025,1,1));
	//DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	//ti_cbo.Set(cmds[0], Mat, 0);
	//mesh[4].Draw(cmds[0]);
	pipe[0].End();

	cmds[2] = pipe[2].Begin();
	model = obj.plots[0].mat;
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	mvpCBO.Set(cmds[2], Mat, 0);
	mesh[obj.shapeID].Draw(cmds[2]);
	pipe[2].End();

	//model = GenModelMatrix(point(0, 0, 0, 10), point(0, 0, 0, 0), point(1, 1, 1, 1));
	//DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	cmds[1] = pipe[1].Begin();
	//offscreen_cbo.Set(cmds[1], Mat, 0);
	//ino::d3d::renderOffscreen.Set(cmds[1], 1);
	//mesh[1].Draw(cmds[1]);
	pipe[1].End();

	ino::d3d::excute(cmds, _countof(pipe));
	ino::d3d::wait();
	ino::d3d::end();
	return TRUE;
}

DLL_EXPORT int MeshCount()
{
	return 0;// mesh.size();
}

DLL_EXPORT void SetTime(UINT t_)
{
	t = t_/frameTime;

	if (plotMode == 0)
		return;

	camPlotsIndex = 0;
	while (camPlots.size() > camPlotsIndex + 1)
	{
		Camera c1 = camPlots[camPlotsIndex];
		Camera c2 = camPlots[camPlotsIndex + 1];

		if (c2.plotTime < t)
		{
			camPlotsIndex += 1;
			continue;
		}
		break;
	}
}

DLL_EXPORT void SetPlotMode(UINT isPlot)
{
	plotMode = isPlot;
}

DLL_EXPORT void SetCurrentCamera(point p, point t, point u)
{
	cam.pos = Point2XmVec(p);
	cam.target = Point2XmVec(t);
	cam.up = Point2XmVec(u);
}

DLL_EXPORT void AddPlotCamera(UINT id, point pos, point tar, point up,UINT t)
{
	Camera c = {
		.pos = Point2XmVec(pos),
		.target = Point2XmVec(tar),
		.up = Point2XmVec(up),
		.plotTime = t/frameTime
	};

	if (id > camPlots.size() && camPlots.size() != 0 )
		camPlots.insert(camPlots.begin(),c);
	else if( id > camPlots.size() )
		camPlots.push_back(c);
	else
		camPlots.insert(camPlots.begin()+id+1,c);
}

DLL_EXPORT void DelPlotCamera(UINT id)
{
	if (id < camPlots.size())
		camPlots.erase(camPlots.begin()+id);
}

DLL_EXPORT point GetCurrentCameraPos(UINT id)
{
	if (id < camPlots.size())
		return XMVec2Point(camPlots[id].pos);
	return point{};
}

DLL_EXPORT point GetCurrentCameraTar(UINT id)
{
	if (id < camPlots.size())
		return XMVec2Point(camPlots[id].target);
	return point{0,0,-1,0};
}

DLL_EXPORT point GetCurrentCameraUp(UINT id)
{
	if (id < camPlots.size())
		return XMVec2Point(camPlots[id].up);
	return point{ 0,1,0,0 };
}

DLL_EXPORT UINT GetCurrentCameraTime(UINT id)
{
	if (id < camPlots.size())
		return static_cast<UINT>(camPlots[id].plotTime*frameTime);
	return 0;
}

DLL_EXPORT void SetCurrentObject(UINT shape, point pos, point rot,point sc)
{
	namespace DX = DirectX;
	obj.shapeID = shape;
	obj.plots[0].mat = GenModelMatrix(pos, rot, sc);
}

DLL_EXPORT BOOL ExistObject(UINT id)
{
	if (id < objPlots.size())
		return TRUE;
	return FALSE;
}

DLL_EXPORT void AddObject(UINT shape)
{
	DrawObject obj = {
		.shapeID = shape,
	};

	obj.plots[0].scale = DirectX::XMVectorSet(1, 1, 1, 1);
	obj.cbo.Create();
	objPlots.push_back(obj);
}

DLL_EXPORT void SetObject(
	UINT objectID,UINT index,
	point pos, point rot, point sc, UINT ti)
{
	if (objectID < objPlots.size())
	{
		objPlots[objectID].plots[index].mat = GenModelMatrix(pos, rot, sc);
		objPlots[objectID].plots[index].Time = ti / frameTime;

		objPlots[objectID].plots[index].pos = Point2XmVec(pos);
		objPlots[objectID].plots[index].rot = Point2XmVec(rot);
		objPlots[objectID].plots[index].scale = Point2XmVec(sc);

		objPlots[objectID].plotCount = std::max(index+1, objPlots[objectID].plotCount);
		objPlots[objectID].plotTimeEnd = std::max(objPlots[objectID].plotTimeEnd,ti/frameTime);
	}
}

DLL_EXPORT void DelObject(UINT objectID)
{
	if ( objectID < objPlots.size() )
		objPlots.erase(objPlots.begin()+objectID);
}

DLL_EXPORT void DelPlot(UINT objectID,UINT index)
{
	if (objPlots[objectID].plotCount > index)
	{
		for (int i = index; i < objPlots[objectID].plotCount - 1; ++i)
			objPlots[objectID].plots[i] = objPlots[objectID].plots[i + 1];
		objPlots[objectID].plotCount -= 1;

		float t = 0.0f;
		for (int i = 0; i < objPlots[objectID].plotCount; ++i)
		{
			t = std::max(objPlots[objectID].plotTimeEnd,t);
		}
		objPlots[objectID].plotTimeEnd = t;
	}
}

DLL_EXPORT UINT PlotCount(UINT objectID)
{
	if (objPlots.size() <= objectID)
		return 0;
	return objPlots[objectID].plotCount;
}

DLL_EXPORT void SetPlotShape(UINT id,UINT shapeID)
{
	if (id < objPlots.size())
		objPlots[id].shapeID = shapeID;
}

DLL_EXPORT UINT GetPlotShape(UINT id)
{
	if (id < objPlots.size())
		return objPlots[id].shapeID;
	return 0;
}

DLL_EXPORT point GetPlotPos(UINT id, UINT index)
{
	if (id < objPlots.size())
		return XMVec2Point( objPlots[id].plots[index].pos );
	return point{};
}

DLL_EXPORT point GetPlotRot(UINT id,UINT index)
{
	if (id < objPlots.size())
		return XMVec2Point( objPlots[id].plots[index].rot );
	return point{};
}

DLL_EXPORT point GetPlotScale(UINT id,UINT index)
{
	if (id < objPlots.size())
		return XMVec2Point(objPlots[id].plots[index].scale);
	return point{};
}

DLL_EXPORT UINT GetPlotTime(UINT id,UINT index)
{
	if (id < objPlots.size())
		return ( objPlots[id].plots[index].Time) * frameTime;
	return 0;
}

DLL_EXPORT void OpenWindow()
{
	MessageBox(0, L"Woww", L"", MB_OK);
}
