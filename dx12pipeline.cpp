#include "dx12pipeline.hpp"

namespace ino::d3d {

LPCSTR constexpr pipeline::getCstrShaderType(ShaderTypes type) const
{
	switch (type)
	{
	case ShaderTypes::VERTEX_SHADER:
		return "vs_5_0";
		break;
	case ShaderTypes::FRAGMENT_SHADER:
		return "ps_5_0";
		break;
	case ShaderTypes::GEOMETORY_SHADER:
		return "gs_5_0";
		break;
	case ShaderTypes::DOMAIN_SHADER:
		return "ds_5_0";
		break;
	case ShaderTypes::HULL_SHADER:
		return "hs_5_0";
		break;
	case ShaderTypes::UNDEFINED:
		return "unbdefined";
		break;
	}
	return "unbdefined";
}

pipeline::pipeline()
{
}

pipeline::~pipeline()
{
	for (auto& s : shader)
		if (s) s->Release();
}

void pipeline::LoadShader(ShaderTypes type, std::wstring_view path, LPCSTR entryPoint)
{
	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3DCompileFromFile(
		path.data(),
		nullptr/*def*/,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		getCstrShaderType(type),
		compile_flag, 0,
		&shader[static_cast<int>(type)], &error);

	putErrorMsg(error);
}

void pipeline::LoadShader(ShaderTypes type, void* src, size_t src_size, LPCSTR entryPoint)
{
	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3DCompile(
		src,
		src_size,
		nullptr/*name*/,
		nullptr/*def*/,
		nullptr/*include*/,
		entryPoint,
		getCstrShaderType(type),
		compile_flag,
		0, &shader[static_cast<int>(type)], &error);
	putErrorMsg(error);
}

void pipeline::Create(
	D3D12_INPUT_ELEMENT_DESC* elementDescs,
	UINT elemntCount,
	BOOL enableDepth,
	//D3D12_ROOT_PARAMETER const& param,
	D3D12_BLEND_DESC const& blendDesc,
	D3D12_RASTERIZER_DESC const& rasterDesc) {

	/*
	range[0].NumDescriptors     = 1;
	range[0].BaseShaderRegister = 0;
	range[0].RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	root_parameters[1].ParameterType                = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	root_parameters[1].ShaderVisibility             = D3D12_SHADER_VISIBILITY_ALL;
	root_parameters[1].DescriptorTable.NumDescriptorRanges = 1;
	root_parameters[1].DescriptorTable.pDescriptorRanges   = &range[0];

	command_list->SetGraphicsRootDescriptorTable(1, dh_texture_->GetGPUDescriptorHandleForHeapStart());
	*/
	
	// ルートパラメータの設定.
	static D3D12_ROOT_PARAMETER param = {};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	/*
	sampler_desc.Filter             = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW           = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MipLODBias         = 0.0f;
	sampler_desc.MaxAnisotropy      = 16;
	sampler_desc.ComparisonFunc     = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.BorderColor        = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_desc.MinLOD             = 0.0f;
	sampler_desc.MaxLOD             = D3D12_FLOAT32_MAX;
	sampler_desc.ShaderRegister     = 0;
	sampler_desc.RegisterSpace      = 0;
	sampler_desc.ShaderVisibility   = D3D12_SHADER_VISIBILITY_ALL;
	*/

	//Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
		.NumParameters = 1,
		.pParameters = &param,
		.NumStaticSamplers = 0,
		.pStaticSamplers = nullptr,
		.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT 
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
	};


	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	putErrorMsg(error);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
		.pRootSignature = rootSignature.Get(),
		.BlendState = blendDesc,
		.SampleMask = UINT_MAX,
		.RasterizerState = rasterDesc,
		.DepthStencilState = {
			.DepthEnable = enableDepth ? TRUE : FALSE,
			.DepthFunc = enableDepth ? D3D12_COMPARISON_FUNC_GREATER : D3D12_COMPARISON_FUNC_NEVER,
			.StencilEnable = FALSE,
		},
		.InputLayout = { elementDescs, elemntCount },
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM,} ,
		.SampleDesc = {.Count = 1,},
	};

	if (shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)])
		desc.VS = { shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)]->GetBufferPointer() ,shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)]->GetBufferSize() };
	if (shader[static_cast<int>(d3d::ShaderTypes::FRAGMENT_SHADER)])
		desc.PS = { shader[static_cast<int>(d3d::ShaderTypes::FRAGMENT_SHADER)]->GetBufferPointer() ,shader[static_cast<int>(d3d::ShaderTypes::FRAGMENT_SHADER)]->GetBufferSize() };
	if (shader[static_cast<int>(d3d::ShaderTypes::GEOMETORY_SHADER)])
		desc.GS = { shader[static_cast<int>(d3d::ShaderTypes::GEOMETORY_SHADER)]->GetBufferPointer() ,shader[static_cast<int>(d3d::ShaderTypes::GEOMETORY_SHADER)]->GetBufferSize() };
	if (shader[static_cast<int>(d3d::ShaderTypes::DOMAIN_SHADER)])
		desc.DS = { shader[static_cast<int>(d3d::ShaderTypes::DOMAIN_SHADER)]->GetBufferPointer() ,shader[static_cast<int>(d3d::ShaderTypes::DOMAIN_SHADER)]->GetBufferSize() };
	if (shader[static_cast<int>(d3d::ShaderTypes::HULL_SHADER)])
		desc.HS = { shader[static_cast<int>(d3d::ShaderTypes::HULL_SHADER)]->GetBufferPointer() ,shader[static_cast<int>(d3d::ShaderTypes::HULL_SHADER)]->GetBufferSize() };

	device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));
}

::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> const& pipeline::Get()
{
	return commandList;
}

ID3D12GraphicsCommandList* pipeline::Begin()
{
	if (commandList)
	{
		commandList->Reset(commandAllocators[currentBackBufferIndex].Get(), pipelineState.Get());
	}
	else
	{
		auto& commandAllocator = commandAllocators[currentBackBufferIndex];
		device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocators[currentBackBufferIndex].Get(),
			pipelineState.Get(),
			IID_PPV_ARGS(&commandList));
	}

	if (commandList == nullptr)
		return nullptr;

	HRESULT result = NULL;
	auto backBuffer = renderTargets[currentBackBufferIndex];

	D3D12_RESOURCE_BARRIER const clear_barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = backBuffer.Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};

	commandList->ResourceBarrier(1, &clear_barrier);

	rtvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.ptr += renderTargetDescriptorSize * currentBackBufferIndex;

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	commandList->RSSetViewports(1, &view);
	commandList->RSSetScissorRects(1, &scissor);

	return commandList.Get();
}

void pipeline::Clear(FLOAT const clearColor[])
{
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
#ifdef USE_STENCIL_BUFFER
	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(stencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->ClearDepthStencilView(scvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#endif
}

void pipeline::End()
{
	if (commandList == nullptr)
		return;
	auto backBuffer = renderTargets[currentBackBufferIndex];
	// Present
	D3D12_RESOURCE_BARRIER const present_barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = backBuffer.Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
			.StateAfter = D3D12_RESOURCE_STATE_PRESENT,
		},
	};

	commandList->ResourceBarrier(1, &present_barrier);
	commandList->Close();
}

}