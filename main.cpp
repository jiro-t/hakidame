#include "dx.hpp"
#include "dx12pipeline.hpp"
#include "dx12resource.hpp"

#include <thread>
#include <mutex>

float local_time = 0.f;

/*
double SimpleMonteCarlo()
{
	double rangeMin = 0;
	double rangeMax = 3.14159265359;

	size_t numSamples = 10000;

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(rangeMin, rangeMax);

	double ySum = 0.0;
	for (size_t i = 1; i <= numSamples; ++i)
	{
		double x = dist(mt);
		double y = sin(x)*sin(x);
		ySum += y;
	}
	double yAverage = ySum / double(numSamples);

	double width = rangeMax - rangeMin;
	double height = yAverage;

	return width * height;
}
double GeneralMonteCarlo()
{
	size_t numSamples = 10000;

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(0.0f, 1.0f);

	auto InverseCDF = [](double x) -> double
	{
		return x * c_pi;
	};

	auto PDF = [](double x) -> double
	{
		return 1.0f / c_pi;
	};

	double estimateSum = 0.0;
	for (size_t i = 1; i <= numSamples; ++i)
	{
		double rnd = dist(mt);
		double x = InverseCDF(rnd);
		double y = sin(x)*sin(x);
		double pdf = PDF(x);
		double estimate = y / pdf;

		estimateSum += estimate;
	}
	double estimateAverage = estimateSum / double(numSamples);

	return estimateAverage;
}
double ImportanceSampledMonteCarlo()
{
	size_t numSamples = 10000;

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(0.0, 1.0);

	auto InverseCDF = [](double x) -> double
	{
		return 2.0 * asin(sqrt(x));
	};

	auto PDF = [](double x) -> double
	{
		return sin(x) / 2.0f;
	};

	double estimateSum = 0.0;
	for (size_t i = 1; i <= numSamples; ++i)
	{
		double rng = dist(mt);
		double x = InverseCDF(rng);
		double y = sin(x)*sin(x);
		double pdf = PDF(x);
		double estimate = y / pdf;

		estimateSum += estimate;
	}
	double estimateAverage = estimateSum / double(numSamples);

	return estimateAverage;
}
*/

//#include <DirectML.h>
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
cbuffer cbTansMatrix : register(b0){\
float scale;\
};\
struct PSInput {\
	float4	position : SV_POSITION;\
	float4	color : COLOR;\
};\
PSInput VSMain(float3 position : POSITION, float4 color : COLOR)\
{\
	PSInput	result;\
	result.position = float4(position,1) + float4(scale,0,0,0);\
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
	pipe.Create(elementDescs, 2, true);
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

		//std::thread t1([&cmds, &pipe, &m]() {
			//const FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
			m.lock();
			cmds[0] = pipe[0].Begin();
			c1.Set(cmds[0], sinf(local_time));
			m.unlock();
			pipe[0].Clear(clearColor);
			//DrawPrimitive;
			pipe[0].End();// }
		//);

		m.lock();
		cmds[1] = pipe[1].Begin();
		c1.Set(cmds[1],sinf(local_time));
		m.unlock();
		//DrawPrimitive;
		vbo->Draw(cmds[1]);
		pipe[1].End();

		//t1.join();

		ino::d3d::flush(cmds, _countof(pipe));
		Sleep(16);
	} while (!GetAsyncKeyState(VK_ESCAPE));//&& local_time < 144.83f);

	delete vbo;

	ino::d3d::release();
	return 0;
}
