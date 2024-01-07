#include <memory>
#include <sstream>
#include <windows.h>

/*Becky's! SDK fails to compile cleanly with the default warnings level,
* since all members of `CBeckyApi` are initialized in `InitAPI`, rather than in the constructor
* which causes warning 'C26495: 'Variable 'variable' is uninitialized. Always initialize a member variable (type.6).'.
* This code is not ours, so disable this warning temporarily.
*/
#pragma warning( push )
#pragma warning( disable : 26495)

#include "..\include\BeckyApi.h"

#pragma warning( pop ) 


CBeckyAPI B2_API;

HINSTANCE library_instance;


class MessageWindow
{
public:
	~MessageWindow();
	static void ensureExists();
	static void destroy();

private:
	MessageWindow();
	inline static std::unique_ptr<MessageWindow> _instance = nullptr;
	ATOM _classAtom{};
	HWND _windowHandle{};
	static LRESULT CALLBACK windowMsgHandler(HWND, UINT, WPARAM, LPARAM);

};


LRESULT CALLBACK MessageWindow::windowMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg){
	case (WM_USER + 11): {
		CHAR msgid[256] = "alazadara";
		int res = B2_API.GetNextMail(-1, &msgid[0], 256, FALSE);
		std::wostringstream s;
		s << L"Got ID: " << res << L", and message id is: " << msgid;
		OutputDebugString(s.str().c_str());
		return 13;
		}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


MessageWindow::MessageWindow()
{
	WNDCLASSEX clsInfo{
		.cbSize = sizeof(WNDCLASSEX),
		.lpfnWndProc = &windowMsgHandler,
		.hInstance = library_instance,
		.lpszClassName = L"becky2nvda",
	};
	ATOM classRegistrationRes = RegisterClassEx(&clsInfo);
	if (classRegistrationRes == 0) {
		throw L"Failed to register class";
	}
	_classAtom = classRegistrationRes;
	HWND windowCreationRes = CreateWindowEx(
		0, /*dwExStyle*/
		MAKEINTATOM(_classAtom), /*lpClassName*/
		L"becky2nvda", /*lpWindowName*/
		0, /*dwStyle*/
		0, /*X*/
		0, /*Y*/
		0, /*nWidt*/
		0, /*nHeight*/
		HWND_MESSAGE, /*hWndParent*/
		nullptr, /*hMenu*/
		library_instance, /*hInstance*/
		nullptr /*lpParam*/
	);
	if (!windowCreationRes) {
		throw L"Failed to create window";
	}
	_windowHandle = windowCreationRes;
}


MessageWindow::~MessageWindow()
{
	if (_windowHandle) {
		if (0 == DestroyWindow(_windowHandle)) {
			OutputDebugString(L"dfailed to destroy window\n");
		}
		_windowHandle = nullptr;
	}
	if (_classAtom) {
		if (0 == UnregisterClass(MAKEINTATOM(_classAtom), library_instance)) {
			OutputDebugString(L"Failed to unregister class\n");
		}
		_classAtom = 0;
	}
}


void MessageWindow::ensureExists() {
	if (!_instance) {
		_instance = std::unique_ptr<MessageWindow>(new MessageWindow());
	}
}


void MessageWindow::destroy() {
	if (_instance) {
		_instance.reset(nullptr);
	}
}


struct tagBKPLUGININFO
{
    char szPlugInName[80];
    char szVendor[80];
    char szVersion[80];
    char szDescription[256];
};


extern "C" int WINAPI BKC_OnPlugInInfo(tagBKPLUGININFO* lpPlugInInfo){
    strcpy_s(lpPlugInInfo->szPlugInName, 80, "Becky_NVDA_connector");
    strcpy_s(lpPlugInInfo->szVendor, 80, "Lukasz Golonka");
    strcpy_s(lpPlugInInfo->szVersion, 80, "1.0.0");
    strcpy_s(lpPlugInInfo->szDescription, 256, "Interfaces NVDA plug-in with Becky! Internet Mail.");
    return 0;
}


extern "C" int WINAPI BKC_OnStart(){
    MessageWindow::ensureExists();
    return 0;
}


extern "C" int WINAPI BKC_OnExit(){
    MessageWindow::destroy();
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {
		if (B2_API.InitAPI()) {
			library_instance = hModule;
			return TRUE;
		}
		return FALSE;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
