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


int get_msg_code_page(int iacc_child_id) {
	/*
	* Tries to retrieve code page from the message with given ID.
	* It was assumed that it will allow us to get the encoding of mail headers,
	* but API just returns code page of message's body, which we don't care about.
	* While normally these encodings should be the same, they are not way too often making this approach impractical.
	* `GetCharSet` also  fails to retrieve code page from multipart messages where each of the parts uses different encoding
	* and in cases where the entire message was not yet downloaded.
	* We tried some alternatives, none of which were workable:
	* - Retrieve specific headers we care about using `GetSpecifiedHeader` and then get a code page from it - does not work for messages other than the currently focused one
	* - Use `GetHeader` to fetch all headers and retrieve code page name from them - returned headers are already decoded, so information about what code page they were using is lost
	* - Fetch the entire message source with `GetSource` and parse to retrieve code page - works only for fully downloaded messages
	* Given the above this was an interesting exercise in C++, but nothing usable come out of it.
	*/
	std::wostringstream s;
	// There is absolutely no documentation as to how long the name of the message encoding can be.
	// Were going to assume that 256 is sufficient. There is also no way to check if it was fully retrieved,
	// since when buffer is too short the encoding name gets truncated.
	const int MSG_CODE_PAGE_BUFF_SIZE = 256;
	std::string code_page_buff(MSG_CODE_PAGE_BUFF_SIZE, '\0');
	const std::string msg_id = get_msg_id_from_iacc_child_id(iacc_child_id);
	int cp_identifier = B2_API.GetCharSet(msg_id.c_str(), code_page_buff.data(), MSG_CODE_PAGE_BUFF_SIZE);
	if (-1 == cp_identifier) {
		// There is no documentation as to what this error code really means, but if it is returned we failed to retrieve message's charset
		throw std::runtime_error("GetCharSet returned -1");
	}
	// Becky! returns the code page both as an numeric identifier (result of `GetCharSet`) and writes its name to the passed buffer.
	// While name of the code page reflects the real code page as specified in the message source,
	// Becky! apparently does some decoding of her own, so content of the message headers in the list view
	// is encoded using a Windows code page whose identifier is returned from `GetCharSet`.
	// The only cases in which code page name is useful for us is to verify that we didn't ask for message which has not yet been downloaded,
	// nor for an one with multiple parts, each specifying their own code page.
	// In these cases Becky! returns an empty name,
	// yet the numeric code page identifier returned is the default  ANSI code page set in Windows properties.
	if (code_page_buff.empty()) {
		throw std::runtime_error("Got empty code page name.");
	}
	return cp_identifier;
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
	static const unsigned int WM_GET_MESSAGE_CHARSET = (WM_USER + 2);

};


LRESULT CALLBACK MessageWindow::windowMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg){
	case WM_GET_MESSAGE_CHARSET: {
		try
		{
			return get_msg_code_page(wParam);
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
