#define WIN32_LEAN_AND_MEAN

#include <include/directx/d3dx12.h>

#pragma comment(lib, "d3d12.lib")

#include <utils/utils.h>
#include <helpers.h>

#include <wrl.h>
#include <d3d12.h>


void Utils::UpdatePlacedBufferResource(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12Heap> heap, UINT64 heapOffset, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** p_destinationResource,
	ID3D12Resource** p_intermediateResource, size_t numberOfElements, size_t sizeOfElements, const void* bufferData) {

    //destination resource
    D3D12_RESOURCE_DESC destinationResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements, D3D12_RESOURCE_FLAG_NONE, 0);
    ThrowIfFailed(device->CreatePlacedResource(heap.Get(), heapOffset * D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, &destinationResourceDescription,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(p_destinationResource)));

    //intermediate resource
    D3D12_RESOURCE_DESC intermediateResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements, D3D12_RESOURCE_FLAG_NONE, 0);
    D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &intermediateResourceDescription,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(p_intermediateResource)));

    //buffer data
    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = bufferData;
    subresourceData.RowPitch = numberOfElements * sizeOfElements;
    subresourceData.SlicePitch = subresourceData.RowPitch;

    //record copy commands on command list
    UpdateSubresources(commandList.Get(), *p_destinationResource, *p_intermediateResource, 0, 0, 1, &subresourceData);

}

void Utils::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** p_destinationResource,
    ID3D12Resource** p_intermediateResource, size_t numberOfElements, size_t sizeOfElements, const void* bufferData) {

    //Create destination resource
    D3D12_HEAP_PROPERTIES destinationResourceHP = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC destinationResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements);

    ThrowIfFailed(device->CreateCommittedResource(&destinationResourceHP, D3D12_HEAP_FLAG_NONE, &destinationResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(p_destinationResource)));

    //Create intermediate resource
    D3D12_HEAP_PROPERTIES intermediateResourceHP = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC intermediateResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfElements * sizeOfElements);

    ThrowIfFailed(device->CreateCommittedResource(&intermediateResourceHP, D3D12_HEAP_FLAG_NONE, &intermediateResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(p_intermediateResource)));

    //Buffer data
    D3D12_SUBRESOURCE_DATA d3d12SubresourceData = {};
    d3d12SubresourceData.pData = bufferData;
    d3d12SubresourceData.RowPitch = numberOfElements * sizeOfElements;
    d3d12SubresourceData.SlicePitch = d3d12SubresourceData.RowPitch;

    //Record commands on commandList
    UpdateSubresources(commandList.Get(), *p_destinationResource, *p_intermediateResource, 0, 0, 1, &d3d12SubresourceData);

}



