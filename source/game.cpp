#define WIN32_LEAN_AND_MEAN

#include <include/directx/d3dx12.h>
#include <DirectXTex/DirectXTex.h>

#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "DirectXTex.lib")

#include <game.h>
#include <application.h>
#include <helpers.h>
#include <commandqueue.h>
#include <objects.h>
#include <utils/utils.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <filesystem>
#include <exception>

namespace fs = std::filesystem;

template <typename T>
constexpr const T& clamp(const T& value, const T& min, const T& max) {

    return value < min ? min : value > max ? max : value;

}

Game::Game() {

    CubeInfo g_cubeInfo;

	m_device = Application::Get()->GetDevice();
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
    m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_fov = 45.0;

    //Draw cubes instanced
    {
        float xShift[] = { 0.0f };
        float yShift[] = { 0.0f };
        float zShift[] = { 0.0f };
        Objects::PrepareBundleDraw(g_cubeInfo.cubeVertex, VERTEX_COUNT_CUBE, g_cubeInfo.cubeIndex, INDEX_COUNT_CUBE, 
            &m_vertices.p_vertices, m_vertices.c_vertices, &m_indicies.p_indicies, m_indicies.c_indicies, 1, xShift, yShift, zShift);
    }
    
    {
        float xShift[] = { 0.0f, 3.0f, 0.0f, 3.0f };
        float yShift[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float zShift[] = { 0.0f, 0.0f, 5.0f, 5.0f };
        Objects::PrepareInstancedDraw(&m_instances.p_instances, m_instances.c_instances, 4, xShift, yShift, zShift);
    }

}

void Game::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> d3d12CommandList, ID3D12Resource** p_destinationResource, ID3D12Resource** p_intermediateResource,
    const void* bufferData, size_t numberOfElements, size_t sizeOfElements) {

    D3D12_HEAP_PROPERTIES destinationResourceHP = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC destinationResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements);

	ThrowIfFailed(m_device->CreateCommittedResource(&destinationResourceHP, D3D12_HEAP_FLAG_NONE, &destinationResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(p_destinationResource)));

    if (bufferData) {

        D3D12_HEAP_PROPERTIES intermediateResourceHP = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements);

        ThrowIfFailed(m_device->CreateCommittedResource(&intermediateResourceHP, D3D12_HEAP_FLAG_NONE, &intermediateResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(p_intermediateResource)));

        D3D12_SUBRESOURCE_DATA d3d12SubresourceData = {};

        d3d12SubresourceData.pData = bufferData;
        d3d12SubresourceData.RowPitch = numberOfElements * sizeOfElements;
        d3d12SubresourceData.SlicePitch = d3d12SubresourceData.RowPitch;
        

        UpdateSubresources(d3d12CommandList.Get(), *p_destinationResource, *p_intermediateResource, 0, 0, 1, &d3d12SubresourceData);

    }

}

void Game::LoadContent() {

    std::shared_ptr<CommandQueue> commandQueue = Application::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = commandQueue->GetCommandList();

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
    ThrowIfFailed(m_device->CreateHeap(&heapDescription, IID_PPV_ARGS(&m_bufferHeap)));

    //Create copy resource
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediatePlacedVBResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediatePlacedIBResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediatePlacedINSTBResource;

    //Staging vertex, index and instance buffers for GPU memory
    Utils::UpdatePlacedBufferResource(m_device, m_bufferHeap, 0, commandList, &m_placedVertexBuffer, &intermediatePlacedVBResource, m_vertices.c_vertices, sizeof(VertexInfo), m_vertices.p_vertices);
    Utils::UpdatePlacedBufferResource(m_device, m_bufferHeap, 1, commandList, &m_placedIndexBuffer, &intermediatePlacedIBResource, m_indicies.c_indicies, sizeof(WORD), m_indicies.p_indicies);
    Utils::UpdatePlacedBufferResource(m_device, m_bufferHeap, 2, commandList, &m_placedInstanceBuffer, &intermediatePlacedINSTBResource, m_instances.c_instances, sizeof(DirectX::XMFLOAT3), m_instances.p_instances);

    //Create vertex buffer view
    m_vertexBufferView.BufferLocation = m_placedVertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = sizeof(VertexInfo) * m_vertices.c_vertices;
    m_vertexBufferView.StrideInBytes = sizeof(VertexInfo);

    //Create index buffer view
    m_indexBufferView.BufferLocation = m_placedIndexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.SizeInBytes = sizeof(WORD) * m_indicies.c_indicies;
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    //Create instance buffer view
    m_instanceBufferView.BufferLocation = m_placedInstanceBuffer->GetGPUVirtualAddress();
    m_instanceBufferView.SizeInBytes = sizeof(DirectX::XMFLOAT3) * m_instances.c_instances;
    m_instanceBufferView.StrideInBytes = sizeof(DirectX::XMFLOAT3);

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDescriptorHeapDesc.NodeMask = 0;
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap)));

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;

    ThrowIfFailed(D3DReadFileToBlob(L"./VertexShader.cso", &vertexShaderBlob));
    ThrowIfFailed(D3DReadFileToBlob(L"./PixelShader.cso", &pixelShaderBlob));

    //Load texture from file into GPU memory
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateTextureResource;
    m_textureBuffer = LoadTexture(commandList, &intermediateTextureResource);

    //Create descriptor shader resource view descriptor heap
    CreateSRVDescriptorHeap();

    //Comment out if using static sampler
    CreateSampler();

    D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXPOSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"INSTANCE_SHIFT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureRootSignature = {};
    featureRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureRootSignature, sizeof(featureRootSignature)))) {

        featureRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRange;

    descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    //Sapmler root parameter
    CD3DX12_DESCRIPTOR_RANGE1 samplerDescriptorRange;
    samplerDescriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[4];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
    //Sampler root parameter
    rootParameters[2].InitAsDescriptorTable(1, &samplerDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);


    const CD3DX12_STATIC_SAMPLER_DESC staticSamplerDescription(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f, 1, D3D12_COMPARISON_FUNC_ALWAYS);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    //Dont use static sampler
    //rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &staticSamplerDescription, rootSignatureFlags);

    //Using non-static sampler
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureErrorBlob;

    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureRootSignature.HighestVersion, &rootSignatureBlob, &rootSignatureErrorBlob));
    ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    struct PipelineStateStream {

        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pssRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT pssInputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY pssPrimitiveTopology;
        CD3DX12_PIPELINE_STATE_STREAM_VS pssVS;
        CD3DX12_PIPELINE_STATE_STREAM_PS pssPS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT pssDepthStencilFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS pssRenderTargetFormats;

    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtFormatArray = {};
    rtFormatArray.NumRenderTargets = 1;
    rtFormatArray.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pssRootSignature = m_rootSignature.Get();
    pipelineStateStream.pssInputLayout = {inputElementDesc, _countof(inputElementDesc)};
    pipelineStateStream.pssPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.pssVS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.pssPS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.pssDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.pssRenderTargetFormats = rtFormatArray;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(PipelineStateStream), &pipelineStateStream};

    ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipelineState)));

    uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    m_contentLoaded = true;

    CreateDepthBuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    GenerateMips();

}

void Game::GenerateMips() {

    //D3D12_DESCRIPTOR_HEAP_DESC uavDHDescription = {};
    //D3D12_DESCRIPTOR_HEAP_DESC srvDHDescription = {};

    //uavDHDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //uavDHDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //uavDHDescription.NodeMask = 0;
    //uavDHDescription.NumDescriptors = 1;

    //srvDHDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //srvDHDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //srvDHDescription.NodeMask = 0;
    //srvDHDescription.NumDescriptors = 1;

    //ThrowIfFailed(m_device->CreateDescriptorHeap(&uavDHDescription, IID_PPV_ARGS(&m_mipGenUAVDescriptorHeap)));
    //ThrowIfFailed(m_device->CreateDescriptorHeap(&srvDHDescription, IID_PPV_ARGS(&m_mipGenSRVDescriptorHeap)));

    //D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescription = {};
    //D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription = {};

    //uavDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //uavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    //uavDescription.Texture2D.MipSlice = 1;

    //srvDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    //srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    //srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //srvDescription.Texture2D.MipLevels = 1;



}

void Game::CreateDepthBuffer(uint64_t width, uint64_t height) {

    if (m_contentLoaded) {

        Application::Get()->Flush();

        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil = { 1.0f, 0 };

        ThrowIfFailed(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_depthBuffer)));

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
        depthStencilDesc.Texture2D.MipSlice = 0;

        m_device->CreateDepthStencilView(m_depthBuffer.Get(), &depthStencilDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    }

}

void Game::OnRender() {
    
    if (m_contentLoaded) {

        auto commandQueue = Application::Get()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        auto commandList = commandQueue->GetCommandList();
        auto backBuffer = Application::Get()->GetCurrentBackBuffer();
        auto renderTargetView = Application::Get()->GetCurrentRenderTargetView();
        auto depthStencilView = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        auto currentBackBufferIndex = Application::Get()->GetCurrentBackBufferIndex();

        {
    
            TransitionResource(backBuffer, commandList, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            FLOAT color[] = {0.4f, 0.6f, 0.9f, 1.0f};

            ClearRTV(renderTargetView, commandList, color);
            ClearDSV(depthStencilView, commandList);
    
        }

        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        D3D12_VERTEX_BUFFER_VIEW p_vBufferViews[] = {m_vertexBufferView, m_instanceBufferView};

        commandList->IASetVertexBuffers(0, 2, p_vBufferViews);
        commandList->IASetIndexBuffer(&m_indexBufferView);

        commandList->RSSetViewports(1, &m_viewport);
        commandList->RSSetScissorRects(1, &m_scissorRect);
    
        commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);

        //DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
        //mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, m_projectionMatrix);

        DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);

        //Set srv descriptor heap and sampler descriptor heap
        ID3D12DescriptorHeap* p_descriptorHeaps[] = {m_srvDescriptorHeap.Get(), m_samplerDescriptorHeap.Get()};

        commandList->SetDescriptorHeaps(2, p_descriptorHeaps);

        commandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX) / 4, &mvpMatrix, 0);
        commandList->SetGraphicsRootDescriptorTable(1, m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->SetGraphicsRootDescriptorTable(2, m_samplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->SetGraphicsRoot32BitConstants(3, 1, &m_angle, 0);

        //Draw non-instanced
        //commandList->DrawIndexedInstanced(m_indicies.c_indicies, 1, 0, 0, 0);

        //Draw instanced
        commandList->DrawIndexedInstanced(m_indicies.c_indicies, 4, 0, 0, 0);

        {
    
            TransitionResource(backBuffer, commandList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

            currentBackBufferIndex = Application::Get()->Present();

            commandQueue->WaitForFenceValue(m_fenceValues[currentBackBufferIndex]);
    
        }
    }

}

void Game::TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
    D3D12_RESOURCE_STATES firstState, D3D12_RESOURCE_STATES secondState) {

    CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), firstState, secondState);
    commandList->ResourceBarrier(1, &resourceBarrier);

}

void Game::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, FLOAT* color) {

    commandList->ClearRenderTargetView(cpuDescriptorHandle, color, 0, nullptr);

}

void Game::ClearDSV(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

    commandList->ClearDepthStencilView(cpuDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

}

void Game::OnUpdate(double time) {

    m_angle = static_cast<float>(time * 360);
    m_angle = DirectX::XMConvertToRadians(m_angle);
    //m_angle = 180.0f;
    //m_angle = 0.0f;

    //const DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(1, 1, 0, 0);
    //m_modelMatrix = DirectX::XMMatrixRotationAxis(rotationAxis, DirectX::XMConvertToRadians(m_angle));
    //m_modelMatrix = DirectX::XMMatrixRotationX(m_angle);
    //m_modelMatrix = DirectX::XMMatrixRotationY(m_angle);

    const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(1.5f, 5.0f, -5.0f, 1);
    //const DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0, 0, 0, 1);
    const DirectX::XMVECTOR eyeDirection = DirectX::XMVectorSet(0, 0, 1, 0);
    const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0, 1, 0, 0);
    //m_viewMatrix = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
    m_viewMatrix = DirectX::XMMatrixLookToLH(eyePosition, eyeDirection, upDirection);

    float aspectRatio = WINDOW_WIDTH / WINDOW_HEIGHT;
    //m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_fov), aspectRatio, 0.1f, 100.0f);
    m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90.0f), aspectRatio, 0.1f, 100.0f);

}

void Game::OnMouseWheel(float delta) {

    m_fov = m_fov - delta;
    m_fov = clamp(m_fov, 15.0f, 90.0f);

}

Microsoft::WRL::ComPtr<ID3D12Resource> Game::LoadTexture(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** p_intermediateResource) {

    std::wstring fileName = L"./texture/directx12texture.dds";

    fs::path filePath(fileName);

    //Check if file exists at path specified
    if (!fs::exists(filePath)) {
        const char* exceptionText = "File path specified doesn't exist.";
        throw std::exception(exceptionText);
    }

    DirectX::ScratchImage scratchImage;
    DirectX::TexMetadata textureMetadata;

    //Load DDS file
    ThrowIfFailed(DirectX::LoadFromDDSFile(filePath.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, &textureMetadata, scratchImage));

    //Change format to sRGB
    textureMetadata.format = DirectX::MakeSRGB(textureMetadata.format);

    TCHAR buffer[500];
    ::swprintf_s(buffer, 500, L"Texture format %d, texture width %d, texture height %d, texture array size %d.\n", textureMetadata.format,
        static_cast<UINT64>(textureMetadata.width), static_cast<UINT64>(textureMetadata.height), static_cast<UINT16>(textureMetadata.arraySize));
    ::OutputDebugString(buffer);
    
    //Create destination resource heap
    D3D12_HEAP_PROPERTIES destinationResourceProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC destinationResourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(textureMetadata.format, static_cast<UINT64>(textureMetadata.width), static_cast<UINT>(textureMetadata.height),
        static_cast<UINT16>(textureMetadata.arraySize), 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    Microsoft::WRL::ComPtr<ID3D12Resource> destinationResource;
    ThrowIfFailed(m_device->CreateCommittedResource(&destinationResourceProperties, D3D12_HEAP_FLAG_NONE, &destinationResourceDescription,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&destinationResource)));

    //Create subresource vector
    std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
    const DirectX::Image* images = scratchImage.GetImages();

    for (uint64_t i{ 0 }; i < scratchImage.GetImageCount(); ++i) {

        auto& subresource = subresources[i];

        subresource.pData = images[i].pixels;
        subresource.RowPitch = images[i].rowPitch;
        subresource.SlicePitch = images[i].slicePitch;

    }

    //Create intermediate resource heap
    UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), 0, static_cast<uint32_t>(subresources.size()));
    D3D12_HEAP_PROPERTIES intermediateResourceProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC intermediateResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
    ThrowIfFailed(m_device->CreateCommittedResource(&intermediateResourceProperties, D3D12_HEAP_FLAG_NONE, &intermediateResourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(p_intermediateResource)));

    UpdateSubresources(commandList.Get(), destinationResource.Get(), *p_intermediateResource, 0, 0, subresources.size(), subresources.data());

    return destinationResource;

}

void Game::CreateSRVDescriptorHeap() {

    D3D12_DESCRIPTOR_HEAP_DESC descrpitorHeapDescription = {};

    descrpitorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descrpitorHeapDescription.NodeMask = 0;
    descrpitorHeapDescription.NumDescriptors = 1;
    descrpitorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    ThrowIfFailed(m_device->CreateDescriptorHeap(&descrpitorHeapDescription, IID_PPV_ARGS(&m_srvDescriptorHeap)));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription = {};

    srvDescription.Format = m_textureBuffer->GetDesc().Format;
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Texture2D.MipLevels = m_textureBuffer->GetDesc().MipLevels;
    srvDescription.Texture2D.MostDetailedMip = 0;
    srvDescription.Texture2D.ResourceMinLODClamp = 0.0f;

    m_device->CreateShaderResourceView(m_textureBuffer.Get(), &srvDescription, m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

void Game::CreateSampler() {

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDescription = {};

    descriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descriptorHeapDescription.NodeMask = 0;
    descriptorHeapDescription.NumDescriptors = 1;
    descriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    ThrowIfFailed(m_device->CreateDescriptorHeap(&descriptorHeapDescription, IID_PPV_ARGS(&m_samplerDescriptorHeap)));

    D3D12_SAMPLER_DESC samplerDescription = {};

    samplerDescription.Filter = D3D12_FILTER_ANISOTROPIC;
    samplerDescription.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.MinLOD = 0.0f;
    samplerDescription.MaxLOD = 0.0f;
    samplerDescription.MipLODBias = 0.0f;
    samplerDescription.MaxAnisotropy = 1;
    samplerDescription.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    m_device->CreateSampler(&samplerDescription, m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

}

Game::~Game() {

    free(m_vertices.p_vertices);
    free(m_indicies.p_indicies);
    free(m_instances.p_instances);

}