#pragma once

#include <wrl.h>
#include <d3d12.h>

class Utils {

public:

	static void UpdatePlacedBufferResource(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12Heap> heap, UINT64 heapOffset, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** p_destinationResource,
		ID3D12Resource** p_intermediateResource, size_t numberOfElements, size_t sizeOfElements, const void* bufferData);
	static void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> d3d12CommandList, ID3D12Resource** p_destinationResource,
		ID3D12Resource** p_intermediateResource, size_t numberOfElements, size_t sizeOfElements, const void* bufferData);

private:

};