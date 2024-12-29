#define WIN32_LEAN_AND_MEAN

#include <include/directx/d3dx12.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <application.h>
#include <helpers.h>
#include <twoPassRender.h>
#include <sphereRender.h>
#include <game.h>
#include <render.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <stdio.h>
#include <memory>
#include <assert.h>
#include <chrono>

static Application* g_applicationInstance = nullptr;
static bool g_isInitialized = false;

Application::Application(HINSTANCE hInstance) {

#if defined(_DEBUG)

	ID3D12Debug* debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();

#endif

	m_windowClass.cbSize = sizeof(WNDCLASSEXW);
	m_windowClass.style = CS_HREDRAW | CS_VREDRAW;
	m_windowClass.lpfnWndProc = WindowProc;
	m_windowClass.cbClsExtra = 0;
	m_windowClass.cbWndExtra = 0;
	m_windowClass.hInstance = hInstance;
	m_windowClass.hIcon = NULL;
	m_windowClass.hCursor = NULL;
	m_windowClass.hbrBackground = (HBRUSH)(COLOR_GRAYTEXT + 1);
	m_windowClass.lpszMenuName = NULL;
	m_windowClass.lpszClassName = L"Direct3dWindowClass";
	m_windowClass.hIconSm = NULL;

	::RegisterClassExW(&m_windowClass);

	m_windowInstance = ::CreateWindowExW(NULL, L"Direct3dWindowClass", L"Direct3d Window", WS_OVERLAPPED | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, nullptr);

	m_allowTearing = CheckforTearingSupport();

	m_dxgiAdapter = GetAdapter();

	if (m_dxgiAdapter) m_d3d12Device = CreateDevice(m_dxgiAdapter);

	m_copyCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);
	m_computeCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_directCommandQueue = std::make_shared<CommandQueue>(m_d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_swapChain = CreateSwapChain();

	m_descriptorHeap = CreateDescriptorHeap();

	CreateRTVs();
	
	m_backBufferSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	g_isInitialized = true;

}

Application* Application::Get() {

	return g_applicationInstance;

}

void Application::Run() {

	m_game = std::make_shared<Game>();
	m_game->LoadContent();

	TCHAR buffer[500];
	::swprintf_s(buffer, 500, L"Game instance created.");
	::OutputDebugString(buffer);

	::ShowWindow(m_windowInstance, SW_SHOW);

	MSG msg = { 0 };

	InitializeUpdateClock();

	while (msg.message != WM_QUIT) {

		::GetMessage(&msg, NULL, 0, 0);
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

	}
	
	::swprintf_s(buffer, 500, L"Window Closed.");
	::OutputDebugString(buffer);

}

void Application::Destroy() {

	if (g_applicationInstance != nullptr) {

		delete(g_applicationInstance);
		g_applicationInstance = nullptr;

	}

}

Application::~Application() {

	Flush();

}

void Application::Flush() {

	m_copyCommandQueue->Flush();
	m_computeCommandQueue->Flush();
	m_directCommandQueue->Flush();

	return;

}

void Application::Create(HINSTANCE hInstance) {

	if (g_applicationInstance == nullptr) {

		g_applicationInstance = new Application(hInstance);

	}

}

HWND Application::GetWindow() {

	return m_windowInstance;

}

bool Application::CheckforTearingSupport() {

	BOOL allowTearing = FALSE;

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory4)))) {

		Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
		if (SUCCEEDED(dxgiFactory4.As(&dxgiFactory5))) {

			if (FAILED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {

				allowTearing = FALSE;

			}

		}

	}

	return allowTearing == TRUE;

}

Microsoft::WRL::ComPtr<IDXGIAdapter4> Application::GetAdapter() {

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;

	Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;

	UINT createFactoryFlags = 0;
	SIZE_T maxDedicatedVideoMemory = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	for (UINT i = 0; dxgiFactory4->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {

		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory) {

			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));

		}

	}

	return dxgiAdapter4;

}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter) {

	Microsoft::WRL::ComPtr<ID3D12Device2> d3d12Device2;

	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&d3d12Device2)));

#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif


	return d3d12Device2;

}

Microsoft::WRL::ComPtr<ID3D12Device2> Application::GetDevice() {

	return m_d3d12Device;

}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Application::CreateSwapChain() {

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory4;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgiSwapChain4;

	UINT createFactoryFlags = false;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = m_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	auto commandQueue = m_directCommandQueue->GetCommandQueue();

	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), m_windowInstance, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
	ThrowIfFailed(dxgiSwapChain1.As(&dxgiSwapChain4));

	m_currentBackBufferIndex = dxgiSwapChain4->GetCurrentBackBufferIndex();

	return dxgiSwapChain4;

}

void Application::CreateRTVs() {

	auto cpuDescriptorHandleOffset = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart());	

	for (uint64_t i{ 0 }; i < FRAME_COUNT; ++i) {

		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		m_d3d12Device->CreateRenderTargetView(backBuffer.Get(), nullptr, cpuDescriptorHandle);

		m_backBuffers[i] = backBuffer;

		cpuDescriptorHandle.Offset(cpuDescriptorHandleOffset);

	}

}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap() {

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};

	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NumDescriptors = FRAME_COUNT;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(m_d3d12Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;

}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandQueueType) {

	std::shared_ptr<CommandQueue> commandQueue;

	switch (commandQueueType) {
		case D3D12_COMMAND_LIST_TYPE_COPY:
			commandQueue = m_copyCommandQueue;
			break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
			commandQueue = m_computeCommandQueue;
			break;
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
			commandQueue = m_directCommandQueue;
			break;
		default:
			assert(false && "Invalid command queue type.");
	}

	return commandQueue;

}

D3D12_CPU_DESCRIPTOR_HANDLE Application::GetCurrentRenderTargetView() {

	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_backBufferSize);

}

Microsoft::WRL::ComPtr<ID3D12Resource> Application::GetCurrentBackBuffer() {

	return m_backBuffers[m_currentBackBufferIndex];

}

uint64_t Application::GetCurrentBackBufferIndex() {

	return m_currentBackBufferIndex;

}

uint64_t Application::Present() {

	ThrowIfFailed(m_swapChain->Present(1, 0));

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	return m_currentBackBufferIndex;

}

void Application::OnRender() {

	m_game->OnRender();

}

void Application::OnUpdate() {

	m_game->OnUpdate(UpdateClockTick());

}

double Application::UpdateClockTick() {

	auto t1 = m_updateClock.now();
	auto delta = t1 - m_t0;
	m_t0 = t1;

	m_elapsedTime = m_elapsedTime + delta.count() * 1e-9;

	if (m_elapsedTime > 1.0f) {
		m_elapsedTime = 0.0;
	}

	return m_elapsedTime;

}

void Application::InitializeUpdateClock() {

	m_t0 = m_updateClock.now();
	m_elapsedTime = 0.0;

}

void Application::OnMouseWheel(float delta) {

	m_game->OnMouseWheel(delta);

}

LRESULT CALLBACK WindowProc(HWND windowInstance, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (g_isInitialized == true) {

		switch (uMsg)
		{
		case WM_DESTROY:
			DestroyWindow(Application::Get()->GetWindow());
			::PostQuitMessage(0);
			break;
		case WM_PAINT:
			Application::Get()->OnUpdate();
			Application::Get()->OnRender();
			break;
		case WM_MOUSEWHEEL:
			{
				float delta = ((short) HIWORD(wParam)) / (float) WHEEL_DELTA;
				Application::Get()->OnMouseWheel(delta);
			}
			break;
		default:
			return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);
		}

	}
	else {

		return ::DefWindowProc(windowInstance, uMsg, wParam, lParam);

	}

}