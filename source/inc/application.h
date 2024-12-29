#pragma once

#include <commandqueue.h>
#include <twoPassRender.h>
#include <game.h>
#include <render.h>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <chrono>

LRESULT CALLBACK WindowProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam);


class Application {

public:

	static void Create(HINSTANCE hInstance);
	static Application* Get();
	void Run();
	static void Destroy();
	HWND GetWindow();
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice();
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandQueueType);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView();
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer();
	uint64_t GetCurrentBackBufferIndex();
	void Flush();
	uint64_t Present();
	void OnRender();
	void OnUpdate();
	void OnMouseWheel(float delta);

private:

	WNDCLASSEXW m_windowClass;
	HWND m_windowInstance;
	bool m_allowTearing;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_backBuffers[FRAME_COUNT];
	std::shared_ptr<CommandQueue> m_copyCommandQueue;
	std::shared_ptr<CommandQueue> m_directCommandQueue;
	std::shared_ptr<CommandQueue> m_computeCommandQueue;
	std::shared_ptr<Render> m_game;
	uint64_t m_currentBackBufferIndex;
	uint64_t m_backBufferSize;
	std::chrono::high_resolution_clock m_updateClock;
	std::chrono::high_resolution_clock::time_point m_t0;
	double m_elapsedTime;

	Application(HINSTANCE hInstance);
	~Application();
	bool CheckforTearingSupport();
	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter();
	Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter);
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();
	void CreateRTVs();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();
	double UpdateClockTick();
	void InitializeUpdateClock();

};
