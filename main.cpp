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

static float g_Vertices2[6][9] = {
	{ 0.0f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 0.0f,1.0f,/**/0.f,0.f }, // 1
	{ 0.f, 1.25f, 0.0f,/**/ 0.0f, 1.0f, 0.0f,1.0f,/**/0.f,1.f }, // 2
	{ 1.25f, 1.25f, 0.0f,/**/ 0.0f, 0.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 1.25f, 1.25f, 0.0f,/**/ 0.0f, 0.0f, 1.0f,1.0f,/**/1.f,1.f }, // 3
	{ 1.25f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 1.0f,1.0f,/**/1.f,0.f }, // 4
	{ 0.0f, 0.f, 0.0f,/**/ 1.0f, 0.0f, 0.0f,1.0f,/**/0.f,0.f } // 1
};

namespace DX = DirectX;
DX::XMVECTOR cornelBox[] = {
	//floor
	DX::XMVectorSet(552.8f, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//ceiling
	DX::XMVectorSet(552.8f, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 0, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//back-wall
	DX::XMVectorSet(549.6f, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(549.6f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//right-wall
	DX::XMVectorSet(0, 0, 559.2f, 1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 0, 0, 1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 548.8f, 0,1),DX::XMVectorSet(0,1.0f,0,1.0f),
	DX::XMVectorSet(0, 548.8f, 559.2f,1),DX::XMVectorSet(0,1.0f,0,1.0f),
	//left-wall
	DX::XMVectorSet(552.8f, 0, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(549.6f, 0, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 559.2f,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	DX::XMVectorSet(556.0f, 548.8f, 0,1),DX::XMVectorSet(1.0f,0,0,1.0f),
	//rectangle - 1-1
	DX::XMVectorSet(130.0f, 165.0f,  65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f,  165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-2
	DX::XMVectorSet(290.0f,   0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-3
	DX::XMVectorSet(130.0f, 0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 165.0f, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(290.0f, 0, 114.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-4
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f, 165.0f, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(130.0f,  0, 65.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 1-5
	DX::XMVectorSet(240.0f, 0, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(240.0f, 165.0f, 272.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 165.0f, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(82.0f, 0, 225.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),

	//rectangle - 2-1
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-2
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-3
	DX::XMVectorSet(472.0f, 0, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(472.0f, 330.0f, 406.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-4
	DX::XMVectorSet(314.0f, 0, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(314.0f, 330.0f, 456.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	//rectangle - 2-5
	DX::XMVectorSet(265.0f, 0, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(265.0f, 330.0f, 296.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 330.0f, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
	DX::XMVectorSet(423.0f, 0, 247.0f,1),DX::XMVectorSet(1.0f,1.0f,1.0f,1.0f),
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
	result.position = mul(float4(position.xyz*0.01f,1),mvp);\
	result.color = color;\
	return result;\
}\
[RootSignature(rootSig)]\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color;\
}\
";

char tex_shader[] =
"#define rootSig \"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT),CBV(b0),CBV(b1),DescriptorTable(SRV(t0)),StaticSampler(s0) \"\n \
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float)*3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float)*3+sizeof(float)*4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

HRESULT create_pipeline_tex_gen1(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, tex_gen_shader1, sizeof(tex_gen_shader1), L"VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, tex_gen_shader1, sizeof(tex_gen_shader1), L"PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,sizeof(float) * 3 + sizeof(float) * 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
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

static BYTE texData[] = {
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,	0xff,0x9f,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0x4f,	0xff,0x8f,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0x3f,	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,	0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,
};

class WaveSound {
	int fs=0; //サンプリング周波数
	int bits=0; //量子化bit数
	int L=0; //データ長

	std::vector<double> data;

	HWAVEOUT handle;
	WAVEHDR  header;
	WAVEFORMATEX formatter;
public:
	WaveSound()
	{
		MMRESULT result = waveOutOpen(&handle, WAVE_MAPPER, &formatter, 0, 0, CALLBACK_NULL);
	}
	~WaveSound()
	{
		waveOutReset(handle);
		waveOutUnprepareHeader(handle, &header, sizeof(WAVEHDR));
		waveOutClose(handle);
	}

	void Load(std::istream& input)
	{
		char header_ID[4];
		long header_size;
		char header_type[4];
		char fmt_ID[4];
		long fmt_size;
		short fmt_format;
		short fmt_channel;
		long fmt_samples_per_sec;
		long fmt_bytes_per_sec;
		short fmt_block_size;
		short fmt_bits_per_sample;
		char data_ID[4];
		long data_size;
		short data_data;

		//wavデータ読み込み
		input.read((char*)&header_ID[0], 4);
		input.read((char*)&header_size, 4);
		input.read((char*)&header_type[0], 4);
		input.read((char*)&fmt_ID[0], 4);
		input.read((char*)&fmt_size, 4);
		input.read((char*)&fmt_format, 2);
		input.read((char*)&fmt_channel, 2);
		input.read((char*)&fmt_samples_per_sec, 4);
		input.read((char*)&fmt_bytes_per_sec, 4);
		input.read((char*)&fmt_block_size, 2);
		input.read((char*)&fmt_bits_per_sample, 2);
		input.read((char*)&data_ID[0], 4);
		input.read((char*)&data_size, 4);

		//パラメータ
		fs = fmt_samples_per_sec;
		bits = fmt_bits_per_sample;
		L = data_size / 2;

		//音声データ
		data.resize(L * sizeof(double));
		for (int n = 0; n < L; n++) {
			input.read((char*)&data_data, 2);

			data[n] = (double)data_data;
		}
	}

	void Play()
	{
		header.lpData = (LPSTR)&data[0];
		header.dwBufferLength = L;
		header.dwFlags = 0;

		waveOutPrepareHeader(handle, &header, sizeof(WAVEHDR));
		waveOutWrite(handle, &header, sizeof(WAVEHDR));
	}
};

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
	ino::d3d::cbo<DX::XMFLOAT4X4> mvpCBO[2];
	ino::d3d::cbo<float> fCBO[2];

	ino::d3d::pipeline pipe[2];
	create_pipeline(pipe[0]);
	create_pipeline_textured(pipe[1]);

	ino::d3d::cbo<float> timeCbo;
	ino::d3d::pipeline offscreenPipe;
	create_pipeline_tex_gen1(offscreenPipe);

	BOOL useDXR = FALSE;
	if (!ino::d3d::CheckDXRSupport(ino::d3d::device))
	{
//		MessageBox(0, L"nthis demo is using DXR!", L"", MB_OK);
//		return 0;
	}

	//resource
	mvpCBO[0].Create();
	mvpCBO[1].Create();
	timeCbo.Create();
	for (auto& val : fCBO)
		val.Create();

	ID3D12GraphicsCommandList* cmds[_countof(pipe) + 1] = {};
	ino::d3d::vbo* vbo = new ino::d3d::vbo();
	vbo->Create(g_Vertices, 9 * sizeof(float), sizeof(g_Vertices));

	ino::d3d::vbo* vbo2 = new ino::d3d::vbo();
	vbo2->Create(g_Vertices2, 9 * sizeof(float), sizeof(g_Vertices2));

	ino::d3d::StaticMesh cornel_box;
	ino::d3d::vbo* vboCornelBox = new ino::d3d::vbo();
	ino::d3d::ibo* iboCornelBox = new ino::d3d::ibo();
	vboCornelBox->Create(cornelBox, 8 * sizeof(float), sizeof(cornelBox));
	iboCornelBox->Create(g_indecies1, sizeof(g_indecies1));
	cornel_box.vbo = *vboCornelBox;
	cornel_box.ibo = *iboCornelBox;

	ino::d3d::texture tex;
	tex.Create(4, 4);
	tex.Map(texData, 4, 4, 4);
	std::chrono::high_resolution_clock c;
	std::chrono::high_resolution_clock::time_point time_o = c.now();
	float rot = 0.0f;

	ino::d3d::begin();
	//Init DXR
	{
		//ino::d3d::dxr::InitDXRDevice();
		//ino::d3d::dxr::AccelerationStructure as;
		//as.AddGeometory(cornel_box);
		//as.BuildBlas();
		//as.AddInstance(0);
		//as.BuildTlas();
		//ino::d3d::dxr::DxrPipeline dxrPipe;
		//dxrPipe.Create();
		//ino::d3d::end();
	}
	Sleep(500);

	//playSound
	auto musicHandle = Music();
	
	srand(0);
	do {
		MSG msg;
		if (PeekMessage(&msg, hwnd, 0, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		auto model = DX::XMMatrixIdentity();
		rot += 0.01f;
		auto view = DX::XMMatrixLookAtLH(DX::XMVectorSet(cos(rot) * 13, 3, sin(rot) * 13, 0), DX::XMVectorSet(0, 1.5, 0, 0), DX::XMVectorSet(0, 1, 0, 0));
		auto projection = DX::XMMatrixPerspectiveFovLH(DX::XMConvertToRadians(60), ino::d3d::screen_width / (float)ino::d3d::screen_height, 0.01f, 100.f);

		DX::XMFLOAT4X4 Mat;
		DX::XMStoreFloat4x4(&Mat, XMMatrixTranspose(model * view * projection));

		local_time += std::chrono::duration_cast<std::chrono::milliseconds>(c.now() - time_o).count() / 1000.f;
		time_o = c.now();
		ino::d3d::begin();

		std::mutex m;

		static const FLOAT clearColor2[] = { 1.f, 1.f, 0.f, 1.0f };
		//offscreen
		cmds[0] = offscreenPipe.Begin(ino::d3d::renderOffscreen);
		offscreenPipe.Clear(ino::d3d::renderOffscreen,clearColor2);
		timeCbo.Set(cmds[0], local_time, 0);
		vbo->Draw(cmds[0]);
		offscreenPipe.End();
		//std::thread t1([&cmds, &pipe,&vboCornelBox, &m,&mvpCBO]() {

				const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 0.0f };
				m.lock();
				cmds[1] = pipe[0].Begin();
				mvpCBO[1].Set(cmds[1], Mat,0);
				pipe[0].Clear(clearColor);
				//DrawPrimitive;
				vboCornelBox->Draw(cmds[1],*iboCornelBox);
				pipe[0].End(); 
				m.unlock();
		//	}
		//);


		m.lock();
		cmds[2] = pipe[1].Begin();
		fCBO[0].Set(cmds[2],sinf(local_time),0);
		fCBO[1].Set(cmds[2],cosf(local_time),1);
		ino::d3d::renderOffscreen.Set(cmds[2],2);
		//DrawPrimitive;
		vbo2->Draw(cmds[2]);
		vbo->Draw(cmds[2]);
		pipe[1].End();
		m.unlock();

		//t1.join();
		//for (auto& c : cmds)c->Close();
		ino::d3d::excute(cmds, _countof(pipe)+1);
		ino::d3d::wait();
		Sleep(16);
		ino::d3d::end();
	} while (!GetAsyncKeyState(VK_ESCAPE));//&& local_time < 144.83f);
	
	Sleep(500);
	delete vbo;
	delete vbo2;
	delete vboCornelBox;
	delete iboCornelBox;

	UnlockResource(musicHandle);
	FreeResource(musicHandle);

	ino::d3d::release();
	return 0;
}
