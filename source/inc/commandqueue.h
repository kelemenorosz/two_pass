#pragma once

#include <helpers.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <queue>

class CommandQueue {

public:

	HANDLE m_fenceEvent;

	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> d3d12Device, D3D12_COMMAND_LIST_TYPE commmandQueueType);
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue();
	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

private:

	struct M_CommandAllocator {

		uint64_t fenceValue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	};

	using CommandAllocatorQueue = std::queue<M_CommandAllocator>;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>>;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	Microsoft::WRL::ComPtr<ID3D12Device2> m_device;
	uint64_t m_fenceValue {0};
	CommandAllocatorQueue m_commandAllocatorQueue;
	D3D12_COMMAND_LIST_TYPE m_commandListType;
	CommandListQueue m_commandListQueue;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator);
	bool IsFenceComplete(uint64_t fenceValue);
	uint64_t Signal();

};
