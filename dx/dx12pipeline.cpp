﻿#include "dx12pipeline.hpp"

#include <fstream>

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

LPCWSTR constexpr pipeline::getWstrShaderType(ShaderTypes type) const
{
	switch (type)
	{
	case ShaderTypes::VERTEX_SHADER:
		return L"vs_6_0";
		break;
	case ShaderTypes::FRAGMENT_SHADER:
		return L"ps_6_0";
		break;
	case ShaderTypes::GEOMETORY_SHADER:
		return L"gs_6_0";
		break;
	case ShaderTypes::DOMAIN_SHADER:
		return L"ds_6_0";
		break;
	case ShaderTypes::HULL_SHADER:
		return L"hs_6_0";
		break;
	case ShaderTypes::UNDEFINED:
		return L"unbdefined";
		break;
	}
	return L"unbdefined";
}

pipeline::pipeline()
{
}

pipeline::~pipeline()
{
	if (params)
	{
		delete[] params;
		params = nullptr;
		Sleep(20);
	}

	if (samplers)
	{
		delete[] samplers;
		samplers = nullptr;
		Sleep(20);
	}

	if (sampler_ranges)
	{
		delete[] sampler_ranges;
		sampler_ranges = nullptr;
		Sleep(20);
	}

	for (auto& s : shader)
	{
		if (s.pShaderBytecode)
		{
			delete[] s.pShaderBytecode;
			s.pShaderBytecode = 0;
		}
	}
}

void pipeline::LoadShader(ShaderTypes type, std::wstring_view path, LPCSTR entryPoint)
{
	::Microsoft::WRL::ComPtr<ID3DBlob> blob;
	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3DCompileFromFile(
		path.data(),
		nullptr/*def*/,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		getCstrShaderType(type),
		hlsl_compile_flag, 0,
		&blob, &error);

	shader[static_cast<int>(type)].BytecodeLength = blob->GetBufferSize();
	void * p = new byte[blob->GetBufferSize()];
	memcpy(p,blob->GetBufferPointer(),blob->GetBufferSize());
	shader[static_cast<int>(type)].pShaderBytecode = p;

	putErrorMsg(error);

	if(blob)blob->Release();
	if(error)error->Release();
}

void pipeline::LoadShader(ShaderTypes type, void* src, size_t src_size, LPCSTR entryPoint)
{
	::Microsoft::WRL::ComPtr<ID3DBlob> blob;
	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	D3DCompile(
		src,
		src_size,
		nullptr/*name*/,
		nullptr/*def*/,
		nullptr/*include*/,
		entryPoint,
		getCstrShaderType(type),
		hlsl_compile_flag,
		0, &blob, &error);

	putErrorMsg(error);

	shader[static_cast<int>(type)].BytecodeLength = blob->GetBufferSize();
	void* p = new byte[blob->GetBufferSize()];
	memcpy(p, blob->GetBufferPointer(), blob->GetBufferSize());
	shader[static_cast<int>(type)].pShaderBytecode = p;

	if(blob)blob->Release();
	if(error)error->Release();
}

void pipeline::LoadShader(ShaderTypes type, void* src, size_t src_size, LPCWSTR entryPoint)
{
	::Microsoft::WRL::ComPtr <IDxcBlob> bc;
	::Microsoft::WRL::ComPtr<IDxcOperationResult> result;

	::Microsoft::WRL::ComPtr <IDxcBlobEncoding> enc;
	lib->CreateBlobWithEncodingOnHeapCopy(src, src_size, 932, &enc);

	compiler->Compile(enc.Get(), L"", entryPoint, getWstrShaderType(type), nullptr, 0, nullptr, 0, nullptr, &result);
	result->GetResult(&bc);
	::Microsoft::WRL::ComPtr <IDxcBlobEncoding> err;
	if (!bc)
	{
		result->GetErrorBuffer(&err);
#ifdef _DEBUG
		std::cout << (char*)err->GetBufferPointer() << std::endl;
#endif
	}

	if (bc->GetBufferSize() <= 0)
	{
#ifdef _DEBUG
		std::cout << (char*)enc->GetBufferPointer() << std::endl;
#endif
	}

	shader[static_cast<int>(type)].BytecodeLength = bc->GetBufferSize();
	void* p = new byte[bc->GetBufferSize()];
	memcpy(p, bc->GetBufferPointer(), bc->GetBufferSize());
	shader[static_cast<int>(type)].pShaderBytecode = p;
}

void pipeline::LoadShader(ShaderTypes type, std::wstring_view path)
{
	std::ifstream ifs;
	ifs.open(path.data(),std::ios::in | std::ios::binary);
	if (!ifs)
		return;
	UINT size = 0;
	char c;

	while (!ifs.eof())
	{
		ifs.read(&c, 1);
		size += 1;
	}

	shader[static_cast<int>(type)].BytecodeLength = size;
	byte* p = new byte[size];
	ifs.close();
	ifs.open(path.data(), std::ios::in | std::ios::binary);
	ifs.read( reinterpret_cast<char*>(p),size);
	shader[static_cast<int>(type)].pShaderBytecode = p;
}

void pipeline::LoadShader(ShaderTypes type,void* src, size_t src_size)
{
	shader[static_cast<int>(type)].BytecodeLength = src_size;
	void* p = new byte[src_size];
	memcpy(p, src, src_size);
	shader[static_cast<int>(type)].pShaderBytecode = p;
}

void pipeline::CreateSampler(
	UINT count, 
	D3D12_FILTER filter[], 
	D3D12_TEXTURE_ADDRESS_MODE warp[])
{
	sampler_count = count;
	samplers = new D3D12_STATIC_SAMPLER_DESC[sampler_count];
	for (std::size_t i = 0; i < count; ++i)
	{
		ZeroMemory(samplers + i, sizeof(D3D12_STATIC_SAMPLER_DESC));

		samplers[i].Filter = filter[i];
		samplers[i].AddressU = warp[i];
		samplers[i].AddressV = warp[i];
		samplers[i].AddressW = warp[i];
		samplers[i].MipLODBias = 0.0f;
		samplers[i].MaxAnisotropy = 16;
		samplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplers[i].MinLOD = 0.0f;
		samplers[i].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[i].ShaderRegister = static_cast<UINT>(i);
		samplers[i].RegisterSpace = 0;
		samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}
}

void pipeline::Create(
	D3D12_INPUT_ELEMENT_DESC* elementDescs,
	UINT elemntCount,
	D3D12_BLEND_DESC const& blendDesc,
	D3D12_RASTERIZER_DESC const& rasterDesc) {
	HRESULT result = S_OK;

	::Microsoft::WRL::ComPtr<ID3DBlob> signature;
	::Microsoft::WRL::ComPtr<ID3DBlob> error;
	
	result = D3DGetBlobPart(shader[static_cast<UINT>(ShaderTypes::VERTEX_SHADER)].pShaderBytecode, shader[static_cast<UINT>(ShaderTypes::VERTEX_SHADER)].BytecodeLength, D3D_BLOB_ROOT_SIGNATURE, 0, &signature);
	device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	putErrorMsg(error);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
		.pRootSignature = rootSignature.Get(),
		.BlendState = blendDesc,
		.SampleMask = UINT_MAX,
		.RasterizerState = rasterDesc,
		.DepthStencilState = {
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
			.FrontFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
			.BackFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
		},
		.InputLayout = { elementDescs, elemntCount },
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { rtvFormat,} ,
		.DSVFormat = DXGI_FORMAT_D32_FLOAT,
		.SampleDesc = {.Count = 1,},
	};

	desc.VS = shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)];
	desc.PS = shader[static_cast<int>(d3d::ShaderTypes::FRAGMENT_SHADER)];
	desc.GS = shader[static_cast<int>(d3d::ShaderTypes::GEOMETORY_SHADER)];
	desc.DS = shader[static_cast<int>(d3d::ShaderTypes::DOMAIN_SHADER)];
	desc.HS = shader[static_cast<int>(d3d::ShaderTypes::HULL_SHADER)];

	device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));

	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	result = device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[currentBackBufferIndex].Get(),
		pipelineState.Get(),
		IID_PPV_ARGS(&commandList));

	commandList->Close();
}

void pipeline::CreateWithSignature(
	D3D12_ROOT_SIGNATURE_DESC* sigDesc,
	D3D12_INPUT_ELEMENT_DESC* elementDescs,
	UINT elemntCount,
	D3D12_BLEND_DESC const& blendDesc,
	D3D12_RASTERIZER_DESC const& rasterDesc) {
	HRESULT result = S_OK;

	::Microsoft::WRL::ComPtr<ID3DBlob> signature;
	::Microsoft::WRL::ComPtr<ID3DBlob> error;

	auto hr = D3D12SerializeRootSignature(sigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, signature.GetAddressOf(),error.GetAddressOf());
	putErrorMsg(error);
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	putErrorMsg(error);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
		.pRootSignature = rootSignature.Get(),
		.BlendState = blendDesc,
		.SampleMask = UINT_MAX,
		.RasterizerState = rasterDesc,
		.DepthStencilState = {
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
			.FrontFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
			.BackFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
		},
		.InputLayout = { elementDescs, elemntCount },
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { rtvFormat,} ,
		.DSVFormat = DXGI_FORMAT_D32_FLOAT,
		.SampleDesc = {.Count = 1,},
	};
	desc.VS = shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)];
	desc.PS = shader[static_cast<int>(d3d::ShaderTypes::FRAGMENT_SHADER)];
	desc.GS = shader[static_cast<int>(d3d::ShaderTypes::GEOMETORY_SHADER)];
	desc.DS = shader[static_cast<int>(d3d::ShaderTypes::DOMAIN_SHADER)];
	desc.HS = shader[static_cast<int>(d3d::ShaderTypes::HULL_SHADER)];

	std::cout << (char*)shader[static_cast<int>(d3d::ShaderTypes::VERTEX_SHADER)].pShaderBytecode << std::endl;

	device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState));

	auto& commandAllocator = commandAllocators[currentBackBufferIndex];
	result = device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[currentBackBufferIndex].Get(),
		pipelineState.Get(),
		IID_PPV_ARGS(&commandList));

	commandList->Close();
}

::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> const& pipeline::Get()
{
	return commandList;
}


ID3D12GraphicsCommandList* pipeline::Begin(renderTexture renderTarget)
{
	commandList->Reset(commandAllocators[currentBackBufferIndex].Get(), pipelineState.Get());

	HRESULT result = NULL;
	//D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = renderTarget.GetHandle().Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};
	commandList->ResourceBarrier(1, &barrier);

#ifdef USE_STENCIL_BUFFER
	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(stencilBuffer[currentBackBufferIndex].GetCpuHeapHandle());
#endif

	renderTarget.RenderTarget(commandList.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->RSSetViewports(1, &view);

	return commandList.Get();
}

ID3D12GraphicsCommandList* pipeline::Begin()
{
	commandList->Reset(commandAllocators[currentBackBufferIndex].Get(), pipelineState.Get());

	HRESULT result = NULL;
	//D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = renderTargets[currentBackBufferIndex].GetHandle().Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
		},
	};
	commandList->ResourceBarrier(1, &barrier);


	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(depthBuffer[currentBackBufferIndex].GetCpuHeapHandle());
	rtvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(renderTargets[currentBackBufferIndex].GetCpuHeapHandle());

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &scvHandle);

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->RSSetViewports(1, &view);
	commandList->RSSetScissorRects(1, &scissor);

	return commandList.Get();
}

void pipeline::Clear(FLOAT const clearColor[])
{
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(depthBuffer[currentBackBufferIndex].GetCpuHeapHandle());
	commandList->ClearDepthStencilView(scvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void pipeline::Clear(renderTexture target,FLOAT const clearColor[])
{
	commandList->ClearRenderTargetView(target.GetRtvHandle(), clearColor, 0, nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(target.GetDsvHandle());
	commandList->ClearDepthStencilView(scvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void pipeline::ClearDepth(renderTexture target)
{
	D3D12_CPU_DESCRIPTOR_HANDLE scvHandle = D3D12_CPU_DESCRIPTOR_HANDLE(target.GetDsvHandle());
	commandList->ClearDepthStencilView(scvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void pipeline::End()
{
	if (commandList == nullptr)
		return;

	std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
	commandList->ResourceBarrier(1, &barrier);
	commandList->Close();
}

shader_resource resource_ptr(UINT idr)
{
	shader_resource result = {};
	HRSRC resInfo = FindResource(0, MAKEINTRESOURCE(idr), L"HLSL");
	HGLOBAL resData = LoadResource(0, resInfo);
	LPVOID pvResData = LockResource(resData);

	result.size = SizeofResource(0, resInfo);
	result.bytecode = std::shared_ptr<void>(malloc(result.size));
	ZeroMemory(result.bytecode.get(),result.size);
	memcpy(result.bytecode.get(), pvResData, result.size);

	UnlockResource(resData);
	FreeResource(resData);

	return result;
}

}
