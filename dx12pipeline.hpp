#ifndef INO_DX_PIPELINE_INCLUDED
#define INO_DX_PIPELINE_INCLUDED

#include "dx.hpp"

namespace ino::d3d {

inline D3D12_BLEND_DESC defaultBlendDesc()
{
	D3D12_BLEND_DESC blendDesc = {
		.AlphaToCoverageEnable = false,
		.IndependentBlendEnable = false,
	};

	for (size_t i = 0; i < 8; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = false;
		blendDesc.RenderTarget[i].LogicOpEnable = false;
		blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	return blendDesc;
}

inline D3D12_RASTERIZER_DESC defaultRasterizerDesc()
{
	D3D12_RASTERIZER_DESC rasterDesc = {
		.FillMode = D3D12_FILL_MODE_SOLID,
		.CullMode = D3D12_CULL_MODE_NONE,
		.FrontCounterClockwise = false,
		.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
		.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		.DepthClipEnable = true,
		.MultisampleEnable = false,
		.AntialiasedLineEnable = false,
		.ForcedSampleCount = 0,
		.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
	};

	return rasterDesc;
}

class pipeline {
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	::Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	::Microsoft::WRL::ComPtr<ID3DBlob> signature;
	::Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	::Microsoft::WRL::ComPtr<ID3DBlob> shader[static_cast<int>(ShaderTypes::UNDEFINED)];
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	static UINT const compile_flag =
#ifdef _DEBUG
		D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION
#else
		D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif
		;

	D3D12_ROOT_PARAMETER* params = nullptr;
	D3D12_STATIC_SAMPLER_DESC* samplers = nullptr;
	D3D12_DESCRIPTOR_RANGE* sampler_ranges = nullptr;
	UINT sampler_count = 0;

	LPCSTR constexpr getCstrShaderType(ShaderTypes type) const;
public:
	D3D12_VIEWPORT view = {};
	D3D12_RECT scissor = {};

	pipeline();
	~pipeline();

	void LoadShader(ShaderTypes type, std::wstring_view path, LPCSTR entryPoint);
	void LoadShader(ShaderTypes type, void* src, size_t src_size, LPCSTR entryPoint);

	void CreateSampler(UINT count, D3D12_FILTER filter[], D3D12_TEXTURE_ADDRESS_MODE warp[]);

	void Create(
		D3D12_INPUT_ELEMENT_DESC* elementDescs,
		UINT elemntCount,
		UINT paramCount,
		BOOL enableDepth,
		D3D12_BLEND_DESC const& blendDesc = defaultBlendDesc(),
		D3D12_RASTERIZER_DESC const& rasterDesc = defaultRasterizerDesc());
	::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> const& Get();

	ID3D12GraphicsCommandList* Begin();
	void Clear(FLOAT const clearColor[]);
	void End();

};

}

#endif