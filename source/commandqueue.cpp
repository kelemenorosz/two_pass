#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <commandqueue.h>
#include <helpers.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#if defined(max)
#undef max
#endif

#include <chrono>

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> d3d12Device, D3D12_COMMAND_LIST_TYPE commandQueueType) {

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};

	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Type = commandQueueType;

	ThrowIfFailed(d3d12Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));
	ThrowIfFailed(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_device = d3d12Device;
	m_commandListType = commandQueueType;

	m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() {

	return m_commandQueue;

}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);

	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const commandListArray[] = {commandList.Get()};

	m_commandQueue->ExecuteCommandLists(_countof(commandListArray), commandListArray);

	uint64_t fenceValue = Signal();

	m_commandListQueue.push(commandList);
	m_commandAllocatorQueue.emplace(M_CommandAllocator{fenceValue, commandAllocator});

	commandAllocator->Release();

	return fenceValue;

}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandQueue::GetCommandList() {

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue)) {

		commandAllocator = m_commandAllocatorQueue.front().commandAllocator;
		m_commandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());

	}
	else {

		commandAllocator = CreateCommandAllocator();
	
	}

	if (!m_commandListQueue.empty()) {

		commandList = m_commandListQueue.front();
		m_commandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

	}
	else {

		commandList = CreateCommandList(commandAllocator);

	}

	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;

}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator) {

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	ThrowIfFailed(m_device->CreateCommandList(0, m_commandListType, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	return commandList;

}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator() {

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;

}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue) {

	return m_fence->GetCompletedValue() >= fenceValue;

}

uint64_t CommandQueue::Signal() {

	m_fenceValue++;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

	return m_fenceValue;

}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue) {

	if (m_fence->GetCompletedValue() < fenceValue) {

		ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
		::WaitForSingleObject(m_fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));

	}

}

void CommandQueue::Flush() {

	WaitForFenceValue(Signal());

}