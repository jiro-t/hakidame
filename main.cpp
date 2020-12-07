#include "dx.hpp"
#include "dx12pipeline.hpp"
#include "dx12resource.hpp"

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

static float g_Vertices[3][7] = {
	{ 0.0f, 0.25f, 0.0f,/**/ 1.0f, 0.0f, 0.0f,1.0f }, // 0
	{ 0.25f, -0.25f, 0.0f,/**/ 0.0f, 1.0f, 0.0f,1.0f }, // 1
	{ -0.25f, -0.25f, 0.0f,/**/ 0.0f, 0.0f, 1.0f,1.0f }, // 2
};

void update() {}

char def_shader[] =
"\
cbuffer cb1 : register(b0){\
float s1;\
};\
cbuffer cb2 : register(b1){\
float s2;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
PSInput VSMain(float3 position : POSITION, float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = float4(position,1) + float4(s1,s2,0,0);\
	result.color = color;\
	return result;\
}\
float4 PSMain(PSInput input) : SV_TARGET\
{\
	return input.color;\
}\
";
HRESULT create_pipeline(ino::d3d::pipeline& pipe)
{
	pipe.LoadShader(ino::d3d::ShaderTypes::VERTEX_SHADER, def_shader, sizeof(def_shader), "VSMain");
	pipe.LoadShader(ino::d3d::ShaderTypes::FRAGMENT_SHADER, def_shader, sizeof(def_shader), "PSMain");

	D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	pipe.Create(elementDescs, 2,2,true);
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
	printf("init=%d", ino::d3d::init(hwnd, true, screen_width, screen_height));
	//[[maybe_unused]] int a;

	//setup pipeline
	ino::d3d::cbo<float> c1;
	ino::d3d::pipeline pipe[2];
	create_pipeline(pipe[0]);
	create_pipeline(pipe[1]);

	c1.Create();
	ID3D12GraphicsCommandList* cmds[_countof(pipe)] = {};
	ino::d3d::vbo* vbo = new ino::d3d::vbo();
	vbo->Create(g_Vertices, 3, 7 * sizeof(float), sizeof(g_Vertices));

	DWORD time_o = ::timeGetTime();
	srand(0);
	do {
		MSG msg;
		if (PeekMessage(&msg, hwnd, 0, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		local_time = (::timeGetTime() - time_o) / 1000.f;

		update();

		const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		std::mutex m;

		std::thread t1([&cmds, &pipe, &m,&c1]() {
			const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
			m.lock();
			cmds[0] = pipe[0].Begin();
			//c1.Set(cmds[0], sinf(local_time));
			pipe[0].Clear(clearColor);
			//DrawPrimitive;
			pipe[0].End(); 
			m.unlock();
			}
		);

		m.lock();
		cmds[1] = pipe[1].Begin();
		c1.Set(cmds[1],sinf(local_time),0);
		c1.Set(cmds[1],sinf(local_time),1);
		//DrawPrimitive;
		vbo->Draw(cmds[1]);
		pipe[1].End();
		m.unlock();

		t1.join();

		ino::d3d::flush(cmds, _countof(pipe));
		Sleep(16);
	} while (!GetAsyncKeyState(VK_ESCAPE));//&& local_time < 144.83f);

	delete vbo;

	ino::d3d::release();
	return 0;
}
