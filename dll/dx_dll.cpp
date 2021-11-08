#define NOMINMAX

#include "../dx/dx.hpp"
#include "../dx/dx12pipeline.hpp"
#include "../dx/dx12resource.hpp"
#include "dx_dll.h"

#include "../gfx/shapes.hpp"
#include "../gfx/font.hpp"

#include <DirectXMath.h>
#include <vector>

#include <iostream>

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

struct DrawObject {
	UINT shapeID;
	DirectX::XMMATRIX matBegin = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX matEnd = DirectX::XMMatrixIdentity();

	float plotTimeBegin;
	float plotTimeEnd;

	DirectX::XMVECTOR posBegin = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR rotBegin = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR scaleBegin = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR posEnd = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR rotEnd = DirectX::XMVectorSet(0, 0, 0, 0);
	DirectX::XMVECTOR scaleEnd = DirectX::XMVectorSet(0, 0, 0, 0);


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
static DrawObject obj;

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
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 3, true);
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

	pipe.Create(elementDescs, 3, false);

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

static ino::d3d::pipeline pipe[2];
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

	camPlots.reserve(2048);
	objPlots.reserve(2048);

	mvpCBO.Create();

	u_cbo.Create();
	n_cbo.Create();
	ti_cbo.Create();
	offscreen_cbo.Create();
	//load default shape
	mesh.push_back( ino::shape::CreateCube() );
	mesh.push_back(ino::shape::CreateQuad());
	mesh.push_back( ino::shape::CreateCharMesh(L'‚¤', L"‚l‚r –¾’©"));
	mesh.push_back(ino::shape::CreateCharMesh(L'‚ñ', L"‚l‚r –¾’©"));
	mesh.push_back(ino::shape::CreateCharMesh(L'‚¿', L"‚l‚r –¾’©"));

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
			const float ab = std::min(1.f,(c2.plotTime - t) / w);
			cam.pos = DX::XMVectorAdd(DX::XMVectorScale(c1.pos, ab), DX::XMVectorScale(c2.pos, 1.f - ab));
			cam.target = DX::XMVectorAdd(DX::XMVectorScale(c1.target, ab), DX::XMVectorScale(c2.target, 1.f - ab));
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
	//cmds[0] = pipe[0].Begin(ino::d3d::renderOffscreen,pipe[0].view, pipe[0].scissor);
	//pipe[0].Clear(clearColor2);
	//pipe[0].End();
	//ino::d3d::excute(cmds, _countof(pipe));
	//ino::d3d::wait();

	//
	cmds[0] = pipe[0].Begin();
	pipe[0].Clear(clearColor);
	for (auto& val : objPlots)
	{
		if (val.plotTimeBegin <= t && val.plotTimeEnd >= t)
		{
			const float w = val.plotTimeEnd - val.plotTimeBegin;
			const float ab = std::min(1.f, (val.plotTimeEnd - t) / w);
			model = val.matBegin*(ab) + val.matEnd*(1.f - ab);
			DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
			
			val.cbo.Set(cmds[0], Mat, 0);
			mesh[0].Draw(cmds[0]);
		}
	}
	model = obj.matBegin;
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	mvpCBO.Set(cmds[0], Mat, 0);
	mesh[0].Draw(cmds[0]);

	model = GenModelMatrix(point(-5, 10, 0, 0), point(0, 0, 0, 0), point(0.025, 0.025, 1, 1));
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	u_cbo.Set(cmds[0], Mat, 0);
	mesh[2].Draw(cmds[0]);

	model = GenModelMatrix(point(0, 10, 0, 0), point(0, 0, 0, 0), point(0.025, 0.025, 1, 1));
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	n_cbo.Set(cmds[0], Mat, 0);
	mesh[3].Draw(cmds[0]);

	model = GenModelMatrix(point(5,10,0,0), point(0,0,0,0), point(0.025,0.025,1,1));
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	ti_cbo.Set(cmds[0], Mat, 0);
	mesh[4].Draw(cmds[0]);
	pipe[0].End();

	model = GenModelMatrix(point(0, 0, 0, 10), point(0, 0, 0, 0), point(1, 1, 1, 1));
	DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));
	cmds[1] = pipe[1].Begin();
	offscreen_cbo.Set(cmds[1], Mat, 0);
	ino::d3d::renderOffscreen.Set(cmds[1], 1);
	mesh[1].Draw(cmds[1]);
	pipe[1].End();

	ino::d3d::excute(cmds, _countof(pipe));
	ino::d3d::wait();
	ino::d3d::end();
	return TRUE;
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

	if( id > camPlots.size() )
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
	obj.matBegin = GenModelMatrix(pos, rot, sc);
}

DLL_EXPORT void AddPlotObject(UINT shape, UINT objectID,point pos, point rot, point sc,UINT ti)
{
	DrawObject obj = {
		.shapeID = shape,
		.matBegin = GenModelMatrix(pos, rot, sc),
		.matEnd = GenModelMatrix(pos, rot, sc),
		.plotTimeBegin = ti / frameTime,
		.plotTimeEnd = ti / frameTime
	};

	obj.posBegin = Point2XmVec(pos);
	obj.rotBegin = Point2XmVec(rot);
	obj.scaleBegin = Point2XmVec(sc);

	obj.posEnd = Point2XmVec(pos);
	obj.rotEnd = Point2XmVec(rot);
	obj.scaleEnd = Point2XmVec(sc);

	obj.cbo.Create();
	
	objPlots.push_back(obj);
}

DLL_EXPORT void SetPlotObject(
	UINT objectID,UINT isBegin,
	point pos, point rot, point sc, UINT ti)
{
	std::wcout << objectID << (isBegin ? L" BEGIN-" : L"END- ") << objPlots.size() << std::endl;
	if (objectID < objPlots.size())
	{
		if (isBegin)
		{
			objPlots[objectID].matBegin = GenModelMatrix(pos, rot, sc);
			objPlots[objectID].plotTimeBegin = ti / frameTime;

			objPlots[objectID].posBegin = Point2XmVec(pos);
			objPlots[objectID].rotBegin = Point2XmVec(rot);
			objPlots[objectID].scaleBegin = Point2XmVec(sc);
		}
		else
		{
			objPlots[objectID].matEnd = GenModelMatrix(pos, rot, sc);
			objPlots[objectID].plotTimeEnd = ti / frameTime;

			objPlots[objectID].posEnd = Point2XmVec(pos);
			objPlots[objectID].rotEnd = Point2XmVec(rot);
			objPlots[objectID].scaleEnd = Point2XmVec(sc);
		}
	}
}

DLL_EXPORT void DelPlotObject(UINT objectID)
{
	if ( objectID < objPlots.size() )
		objPlots.erase(objPlots.begin()+objectID);
}

DLL_EXPORT UINT GetPlotShape(UINT id)
{
	if (id < objPlots.size())
		return objPlots[id].shapeID;
	return 0;
}

DLL_EXPORT point GetPlotPos(UINT id, UINT isBegin)
{
	if (id < objPlots.size())
		return XMVec2Point( isBegin ? objPlots[id].posBegin : objPlots[id].posEnd );
	return point{};
}

DLL_EXPORT point GetPlotRot(UINT id,UINT isBegin)
{
	if (id < objPlots.size())
		return XMVec2Point(isBegin ? objPlots[id].rotBegin : objPlots[id].rotEnd);
	return point{};
}

DLL_EXPORT point GetPlotScale(UINT id,UINT isBegin)
{
	if (id < objPlots.size())
		return XMVec2Point(isBegin ? objPlots[id].scaleBegin : objPlots[id].scaleEnd);
	return point{};
}

DLL_EXPORT UINT GetPlotTime(UINT id,UINT isBegin)
{
	if (id < objPlots.size())
		return (isBegin ? objPlots[id].plotTimeBegin : objPlots[id].plotTimeEnd) * frameTime;
	return 0;
}