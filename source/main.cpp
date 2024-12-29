#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <application.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow) {

	_CrtMemState sOld;
	_CrtMemState sNew;
	_CrtMemState sDiff;
	_CrtMemCheckpoint(&sOld);

	Application::Create(hInstance);
	Application* appInstance = Application::Get();

	appInstance->Run();

	Application::Destroy();

	_CrtMemCheckpoint(&sNew);
	if (_CrtMemDifference(&sDiff, &sOld, &sNew)) {

		::OutputDebugString(L"_CrtMemDumpStatistics");
		_CrtMemDumpStatistics(&sDiff);
		::OutputDebugString(L"_CrtMemDumpAllObjectsSince");
		_CrtMemDumpAllObjectsSince(&sOld);
		::OutputDebugString(L"_CrtDumpMemoryLeaks");
		_CrtDumpMemoryLeaks();

	}

	return 0;

}