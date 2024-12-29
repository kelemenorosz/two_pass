#define WIN32_LEAN_AND_MEAN

#include <include/directx/d3dx12.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "D3Dcompiler.lib")

#include <twoPassRender.h>
#include <objects.h>
#include <application.h>
#include <commandqueue.h>
#include <utils/utils.h>
#include <helpers.h>

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

TwoPassRender::TwoPassRender() {

	CubeInfo g_cubeInfo;

	//Set viewport and scissor rectangle
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
	m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

	//Get ID3D12Device object
	m_device = Application::Get()->GetDevice();

	//Assembling vertex, index and instance buffer data
	{
		float xShift[] = { 0.0f };
		float yShift[] = { 0.0f };
		float zShift[] = { 0.0f };
		Objects::PrepareBundleDraw(g_cubeInfo.cubeVertex, VERTEX_COUNT_CUBE, g_cubeInfo.cubeIndex, INDEX_COUNT_CUBE, 
			&m_vertices.p_vertices, m_vertices.c_vertices, &m_indicies.p_indicies, m_indicies.c_indicies, 1, xShift, yShift, zShift);
	}

	{
		float xShift[] = { -4.0f, 4.0f };
		float yShift[] = { 0.0f, 0.0f };
		float zShift[] = { 0.0f, 0.0f };
		Objects::PrepareInstancedDraw(&m_instances.p_instances, m_instances.c_instances, 2, xShift, yShift, zShift);
	}

}

TwoPassRender::~TwoPassRender() {

	//Free the assembled vertex, index and instance buffer data 
	free(m_vertices.p_vertices);
	free(m_indicies.p_indicies);
	free(m_instances.p_instances);

	//Free constant buffer memory pointer
	if (m_VPConstantBuffer != nullptr) m_VPConstantBuffer->Unmap(0, nullptr);
	mp_VPConstantBuffer = nullptr;

}

void TwoPassRender::LoadContent() {

	LoadIAStage();
	CreateConstantBuffer();
	LoadShaders();
	CreateRootSignature();
	CreatePipelineStateObject();
	ReleaseShaders();
	CreateDepthStencilBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
	CreateTemporaryRenderTarget();
	CreateSampler();

	m_contentLoaded = true;

}

void TwoPassRender::LoadIAStage() {

	//Get copy commandQueue and commandList
	auto commandQueue = Application::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	//Create heap for vertex, index and instance buffers
	D3D12_HEAP_DESC heapDescription = {};
	heapDescription.Flags = D3D12_HEAP_FLAG_NONE;
	heapDescription.SizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 3; // 64KB * 3 for vertex, index and instance buffers
	heapDescription.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heapDescription.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDescription.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDescription.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDescription.Properties.CreationNodeMask = 0;
	heapDescription.Properties.VisibleNodeMask = 0;
	ThrowIfFailed(m_device->CreateHeap(&heapDescription, IID_PPV_ARGS(&m_IAHeap)));

	//Resources used for staging the vertex, index and instance buffers
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateVertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateIndexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateInstanceResource;

	//Place the buffer data in the heap
	Utils::UpdatePlacedBufferResource(m_device, m_IAHeap, 0, commandList, &m_vertexBuffer, &intermediateVertexResource, m_vertices.c_vertices, sizeof(VertexInfo), m_vertices.p_vertices);
	Utils::UpdatePlacedBufferResource(m_device, m_IAHeap, 1, commandList, &m_indexBuffer, &intermediateIndexResource, m_indicies.c_indicies, sizeof(WORD), m_indicies.p_indicies);
	Utils::UpdatePlacedBufferResource(m_device, m_IAHeap, 2, commandList, &m_instanceBuffer, &intermediateInstanceResource, m_instances.c_instances, sizeof(DirectX::XMFLOAT3), m_instances.p_instances);

	//Create vertex, index and instance buffer views
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(VertexInfo) * m_vertices.c_vertices;
	m_vertexBufferView.StrideInBytes = sizeof(VertexInfo);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = sizeof(WORD) * m_indicies.c_indicies;
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

	m_instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
	m_instanceBufferView.SizeInBytes = sizeof(DirectX::XMFLOAT3) * m_instances.c_instances;
	m_instanceBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT3);

	//Execute command list
	commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

}

void TwoPassRender::LoadShaders() {

	ThrowIfFailed(D3DReadFileToBlob(L"./TwoPassVertexShader.cso", &m_vertexShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"./TwoPassPixelShader.cso", &m_firstPassPixelShaderBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"./TwoPassPixelTextureShader.cso", &m_secondPassPixelShaderBlob));

}

void TwoPassRender::ReleaseShaders() {

	m_vertexShaderBlob = nullptr;
	m_firstPassPixelShaderBlob = nullptr;
	m_secondPassPixelShaderBlob = nullptr;

}

void TwoPassRender::CreateConstantBuffer() {

	//Get buffer size rounded up to a multiple of 256
	UINT bufferSize = RoundedConstantBufferSize(sizeof(DirectX::XMMATRIX));

	//Create view-projection matrix resource
	CD3DX12_HEAP_PROPERTIES constantBufferHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC constantBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	ThrowIfFailed(m_device->CreateCommittedResource(&constantBufferHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDescription,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VPConstantBuffer)));

	//Map pointer to resource memory location
	ThrowIfFailed(m_VPConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mp_VPConstantBuffer)));

	//Create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC cbvDescriptorHeapDescription = {};
	cbvDescriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvDescriptorHeapDescription.NodeMask = 0;
	cbvDescriptorHeapDescription.NumDescriptors = 1;
	cbvDescriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvDescriptorHeapDescription, IID_PPV_ARGS(&m_CBVDescriptorHeap)));

	//Create constant buffer view
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDescription = {};
	cbvDescription.BufferLocation = m_VPConstantBuffer->GetGPUVirtualAddress();
	cbvDescription.SizeInBytes = bufferSize;

	m_device->CreateConstantBufferView(&cbvDescription, m_CBVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void TwoPassRender::CreateRootSignature() {

	//Check for root signature version 1.1 support
	D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureVersion = {};
	rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureVersion, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE)))) {
		rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	//First pass root signature
	{

		//Set root parameters
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_VERTEX);
	
		//Root signature description
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		//Binary blob for serialized root signature
		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureErrorBlob;

		//Serialize and create root signature object
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, rootSignatureVersion.HighestVersion, &rootSignatureBlob, &rootSignatureErrorBlob));
		ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_firstPassRootSignature)));

	}

	//Second pass root signature
	{
	
		//Set root parameters
		CD3DX12_DESCRIPTOR_RANGE1 samplerDescriptorRange;
		CD3DX12_DESCRIPTOR_RANGE1 srvDescriptorRange;
		samplerDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		srvDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &srvDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &samplerDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		//Root signature description
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		//Binary blob for serialized root signature
		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureErrorBlob;

		//Serialize and create root signature object
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, rootSignatureVersion.HighestVersion, &rootSignatureBlob, &rootSignatureErrorBlob));
		ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_secondPassRootSignature)));
	
	}
	
}

void TwoPassRender::CreatePipelineStateObject() {

	//Input layout for shaders
	D3D12_INPUT_ELEMENT_DESC inputElementDescription[] = {

		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEX_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"INST_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}

	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDescription = {};
	inputLayoutDescription.pInputElementDescs = inputElementDescription;
	inputLayoutDescription.NumElements = _countof(inputElementDescription);

	//Create first pass pipeline state object
	{

		//Render target formats
		D3D12_RT_FORMAT_ARRAY renderTargetFormat = {};
		renderTargetFormat.NumRenderTargets = 1;
		renderTargetFormat.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		//Create pipeline state object
		struct PipelineStateStream {

			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;

		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_firstPassRootSignature.Get();
		pipelineStateStream.inputLayout = inputLayoutDescription;
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vertexShader = { m_vertexShaderBlob->GetBufferPointer(), m_vertexShaderBlob->GetBufferSize() };
		pipelineStateStream.pixelShader = { m_firstPassPixelShaderBlob->GetBufferPointer(), m_firstPassPixelShaderBlob->GetBufferSize() };
		pipelineStateStream.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.rtvFormats = renderTargetFormat;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDescription = {};
		pipelineStateStreamDescription.SizeInBytes = sizeof(PipelineStateStream);
		pipelineStateStreamDescription.pPipelineStateSubobjectStream = &pipelineStateStream;

		ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDescription, IID_PPV_ARGS(&m_firstPassPipelineState)));

	}

	//Create second pass pipeline state object
	{

		//Render target formats
		D3D12_RT_FORMAT_ARRAY renderTargetFormat = {};
		renderTargetFormat.NumRenderTargets = 1;
		renderTargetFormat.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		//Create pipeline state object
		struct PipelineStateStream {

			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
			CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;

		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_secondPassRootSignature.Get();
		pipelineStateStream.inputLayout = inputLayoutDescription;
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vertexShader = { m_vertexShaderBlob->GetBufferPointer(), m_vertexShaderBlob->GetBufferSize() };
		pipelineStateStream.pixelShader = { m_secondPassPixelShaderBlob->GetBufferPointer(), m_secondPassPixelShaderBlob->GetBufferSize() };
		pipelineStateStream.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.rtvFormats = renderTargetFormat;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDescription = {};
		pipelineStateStreamDescription.SizeInBytes = sizeof(PipelineStateStream);
		pipelineStateStreamDescription.pPipelineStateSubobjectStream = &pipelineStateStream;

		ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDescription, IID_PPV_ARGS(&m_secondPassPipelineState)));

	}

}

void TwoPassRender::CreateDepthStencilBuffer(UINT64 width, UINT height) {

	//Create descriptor heap for depth-stencil view
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
	descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	descriptorHeapDescription.NumDescriptors = 1;
	descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDescription.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&m_DSDescriptorHeap)));

	//Create depth-stencil buffer
	CD3DX12_HEAP_PROPERTIES depthStencilBufferProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthStencilBufferDescription = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	
	D3D12_CLEAR_VALUE depthStencilClearValue = {};
	depthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilClearValue.DepthStencil = { 1.0f, 0 };

	ThrowIfFailed(m_device->CreateCommittedResource(&depthStencilBufferProperties, D3D12_HEAP_FLAG_NONE, &depthStencilBufferDescription,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthStencilClearValue, IID_PPV_ARGS(&m_depthStencilBuffer)));

	//Create depth-stencil view
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescription = {};
	depthStencilViewDescription.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDescription.Flags = D3D12_DSV_FLAG_NONE;
	depthStencilViewDescription.Texture2D.MipSlice = 0;

	m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &depthStencilViewDescription, m_DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void TwoPassRender::CreateTemporaryRenderTarget() {

	//Create temp render target
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 400, 400, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_tempRenderTarget)));
	
	//Create descriptor heap for SRV
	{
	
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
		descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		descriptorHeapDescription.NodeMask = 0;
		descriptorHeapDescription.NumDescriptors = 1;

		ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&m_SRVDescriptorHeap)));
	
	}
	
	//Create descriptor heap for RTV
	{
	
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
		descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapDescription.NodeMask = 0;
		descriptorHeapDescription.NumDescriptors = 1;
		
		ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&m_RTVDescriptorHeap)));

	}

	//Create SRV for temp render target to be used as texture for sampling
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription = {};
	srvDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDescription.Texture2D.MipLevels = 1;
	srvDescription.Texture2D.MostDetailedMip = 0;
	srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;

	m_device->CreateShaderResourceView(m_tempRenderTarget.Get(), &srvDescription, m_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//Create RTV for temp render target
	m_device->CreateRenderTargetView(m_tempRenderTarget.Get(), nullptr, m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


}

void TwoPassRender::CreateSampler() {

	//Create sampler descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};
	descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDescription.NumDescriptors = 1;
	descriptorHeapDescription.NodeMask = 0;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&m_samplerDescriptorHeap)));

	//Create sampler
	D3D12_SAMPLER_DESC samplerDescription = {};
	samplerDescription.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDescription.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescription.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescription.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDescription.MipLODBias = 0.0f;
	samplerDescription.MaxAnisotropy = 16;
	samplerDescription.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	samplerDescription.BorderColor[0] = 1.0f;
	samplerDescription.BorderColor[1] = 1.0f;
	samplerDescription.BorderColor[2] = 1.0f;
	samplerDescription.BorderColor[3] = 1.0f;
	samplerDescription.MinLOD = 0.0f;
	samplerDescription.MaxLOD = 0.0f;

	m_device->CreateSampler(&samplerDescription, m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void TwoPassRender::OnRender() {

	if (!m_contentLoaded) return;

	auto tempRenderTargetView = m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto renderTargetView = Application::Get()->GetCurrentRenderTargetView();
	auto currentBackBuffer = Application::Get()->GetCurrentBackBuffer();
	auto currentBackBufferIndex = Application::Get()->GetCurrentBackBufferIndex();
	auto commandQueue = Application::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();
	auto depthStencilView = m_DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//Clear temp render target and depth stencil buffers
	{

		TransitionResource(m_tempRenderTarget, commandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT color[] = { 0.4f, 0.2f, 0.7f, 1.0f };
		ClearRTV(tempRenderTargetView, commandList, color);
		ClearDSV(depthStencilView, commandList);

	}

	//First pass
	{

		//Set pipeline state and root signatures
		commandList->SetPipelineState(m_firstPassPipelineState.Get());
		commandList->SetGraphicsRootSignature(m_firstPassRootSignature.Get());

		//Set vertex, index and instance buffers
		D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = { m_vertexBufferView, m_instanceBufferView };
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 2, vertexBuffers);
		commandList->IASetIndexBuffer(&m_indexBufferView);

		//Set viewport and scissor rectangle
		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		//Set render targets
		commandList->OMSetRenderTargets(1, &tempRenderTargetView, false, &depthStencilView);

		//Set root parameters
		commandList->SetGraphicsRootConstantBufferView(0, m_VPConstantBuffer->GetGPUVirtualAddress());

		//Draw instanced
		commandList->DrawIndexedInstanced(INDEX_COUNT_CUBE, 2, 0, 0, 0);

	}

	//Transition temp render target to texture resource
	{

		TransitionResource(m_tempRenderTarget, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

	}

	//Clear depth stencil and render target view
	{

		TransitionResource(currentBackBuffer, commandList, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT color[] = { 0.4f, 0.7f, 0.5f, 1.0f };

		ClearRTV(renderTargetView, commandList, color);
		ClearDSV(depthStencilView, commandList);

	}

	//Second pass
	{
	
		//Set pipeline state and root signatures
		commandList->SetPipelineState(m_secondPassPipelineState.Get());
		commandList->SetGraphicsRootSignature(m_secondPassRootSignature.Get());

		//Set vertex, index and instance buffers
		D3D12_VERTEX_BUFFER_VIEW vertexBuffers[] = { m_vertexBufferView, m_instanceBufferView };
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 2, vertexBuffers);
		commandList->IASetIndexBuffer(&m_indexBufferView);

		//Set viewport and scissor rectangle
		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		//Set render targets
		commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

		//Set descriptor heaps used for root parameters
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_SRVDescriptorHeap.Get(), m_samplerDescriptorHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		//Set root parameters
		commandList->SetGraphicsRootConstantBufferView(0, m_VPConstantBuffer->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(1, m_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(2, m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		//Draw instanced
		commandList->DrawIndexedInstanced(INDEX_COUNT_CUBE, 1, 0, 0, 0);
	
	}

	//Execute command list, present to the screen
	{

		TransitionResource(currentBackBuffer, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = Application::Get()->Present();

		commandQueue->WaitForFenceValue(m_fenceValues[currentBackBufferIndex]);

	}

}

void TwoPassRender::TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
	D3D12_RESOURCE_STATES firstState, D3D12_RESOURCE_STATES secondState) {

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), firstState, secondState);
	commandList->ResourceBarrier(1, &resourceBarrier);

}

void TwoPassRender::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT* color) {

	commandList->ClearRenderTargetView(cpuDescriptorHandle, color, 0, nullptr);

}

void TwoPassRender::ClearDSV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	commandList->ClearDepthStencilView(cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

}

void TwoPassRender::OnUpdate(double elapsedTime) {

	//Update view-projection matrix
	DirectX::XMMATRIX viewMatrix;
	DirectX::XMMATRIX projectionMatrix;

	const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
	const DirectX::XMVECTOR eyeDirection = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	viewMatrix = DirectX::XMMatrixLookToLH(eyePosition, eyeDirection, upDirection);

	const float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
	projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90.0f), aspectRatio, 0.1f, 100.0f);

	DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixMultiply(viewMatrix, projectionMatrix);

	memcpy(&mp_VPConstantBuffer[0], &vpMatrix, sizeof(DirectX::XMMATRIX));

}

void TwoPassRender::OnMouseWheel(float mouseWheelDelta) {



}

