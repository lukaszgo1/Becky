#include <memory>
#include <sstream>
#include <stdexcept>
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


void exception_to_debugger(const std::runtime_error& exc_obj) {
	std::wostringstream s;
	s << exc_obj.what();
	OutputDebugStringW(s.str().c_str());
}


std::string get_msg_id_from_iacc_child_id(int iacc_child_id) {
	// Documentation for `GetNextMail` is pretty limited,
	// in particular maximum length for the message ID is not specified, nor can it be retrieved in any way.
	// The only example in the documentation seems to assume it cannot exceed 256 characters, so were going to follow suit.
	const int MSG_ID_MAX_LENGTH = 256;
	std::string msg_id(MSG_ID_MAX_LENGTH, '\0');
	// Becky! indexes messages from 0, whereas IAccessible child ID's are 1 based.
	// There is no function to retrieve message ID based on its index, we just ask about ID of the message after the one with a given index,
	// hence we need to subtract first to convert IAccessible ID to tBecky!'s ID, and then once again to start with an index of a previous message.
	int next_msg_pos = B2_API.GetNextMail((iacc_child_id - 2), msg_id.data(), MSG_ID_MAX_LENGTH, FALSE);
	if (-1 == next_msg_pos) {
		throw std::runtime_error("No next message, this is unexpected");
	}
	if (msg_id.empty()) {
		// When buffer is too short to accomodate message ID Becky! fils it with zeros.
		// If this will become common we can always start with `MSG_ID_MAX_LENGTH`, check if the message ID fits, and if not allocate twice that much until it does.
		throw std::runtime_error("Message buffer too short");
	}
	return msg_id;
}


std::string get_msg_code_page(int iacc_child_id) {
	std::wostringstream s;
	s << L"Id is: " << iacc_child_id;
	OutputDebugStringW(s.str().c_str());
	// There is absolutely no documentation as to how long the name of the message encoding can be.
	// Were going to assume that 256 is sufficient. There is also no way to check if it was fully retrieved,
	// since when buffer is too short the encoding name gets truncated.
	const int MSG_CODE_PAGE_BUFF_SIZE = 256;
	std::string code_page_buff(MSG_CODE_PAGE_BUFF_SIZE, '\0');
	std::string msg_id = get_msg_id_from_iacc_child_id(iacc_child_id);
	s = std::wostringstream();
	s << L"Getting charset for message with id: " << msg_id.c_str();
	OutputDebugStringW(s.str().c_str());
	int cp_identifier = B2_API.GetCharSet(msg_id.c_str(), code_page_buff.data(), MSG_CODE_PAGE_BUFF_SIZE);
	if (-1 == cp_identifier) {
		// There is no documentation as to what this error code really means, but if it is returned we failed to retrieve message's charset
		throw std::runtime_error("GetCharSet returned -1");
	}
	// Becky! returns the code page both as an numeric identifier (result of `GetCharSet`) and writes its name to the passed bufer.
	// Unfortunately the numeric representation is useless, as Becky! tries to be helpful by providing identifier of the Windows code page closest to the actual message charset.
	// For example `1250` is returned for `ISO-8859-2`, which is incorrect.
	s = std::wostringstream();
	s << L"Result code is: " << cp_identifier << L" and code page name is: " << code_page_buff.c_str() << L" the end";
	OutputDebugStringW(s.str().c_str());
	return code_page_buff;
}


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
	static const unsigned int WM_GET_MESSAGE_STATES = (WM_USER + 1);
	static const unsigned int WM_GET_MESSAGE_CHARSET = (WM_USER + 2);

};


LRESULT CALLBACK MessageWindow::windowMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg){
	case WM_GET_MESSAGE_STATES: {
		try
		{
			get_msg_id_from_iacc_child_id(wParam);
			return 1;
		}
		catch (const std::runtime_error& e)
		{
			exception_to_debugger(e);
			return 0;
		}
	}
	case WM_GET_MESSAGE_CHARSET: {
		try
		{
			std::string msg_code_page = get_msg_code_page(wParam);
			strcpy_s(reinterpret_cast<char *>(lParam), (msg_code_page.size() + 1), msg_code_page.c_str());
			return 1;
		}
		catch (const std::runtime_error& e)
		{
			exception_to_debugger(e);
			return 0;
		}
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
		throw std::runtime_error("Failed to register class");
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
		throw std::runtime_error("Failed to create window");
	}
	_windowHandle = windowCreationRes;
}


MessageWindow::~MessageWindow()
{
	if (_windowHandle) {
		if (0 == DestroyWindow(_windowHandle)) {
			OutputDebugStringW(L"dfailed to destroy window\n");
		}
		_windowHandle = nullptr;
	}
	if (_classAtom) {
		if (0 == UnregisterClass(MAKEINTATOM(_classAtom), library_instance)) {
			OutputDebugStringW(L"Failed to unregister class\n");
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
	try
	{
		MessageWindow::ensureExists();
	}
	catch (const std::runtime_error& e)
	{
		exception_to_debugger(e);
	}
	return 0;
}


extern "C" int WINAPI BKC_OnExit(){
    MessageWindow::destroy();
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
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
