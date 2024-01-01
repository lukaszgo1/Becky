///////////////////////////////////////////////////////////////////////////
// Becky! API class library
//
// You can modify and redistribute this file without any permission.

#include <windows.h>
#include "BeckyAPI.h"

BOOL CBeckyAPI::InitAPI()
{
	if (m_hInstBecky) return TRUE;

	BOOL bRet = FALSE;
	do {
		m_hInstBecky = (HINSTANCE)::GetModuleHandle(NULL);
		if (!m_hInstBecky) {
			return FALSE;
		}
		if (!((FARPROC&)GetVersion = GetProcAddress(m_hInstBecky, "BKA_GetVersion"))) break;
		if (!((FARPROC&)Command = GetProcAddress(m_hInstBecky, "BKA_Command"))) break;
		if (!((FARPROC&)GetWindowHandles = GetProcAddress(m_hInstBecky, "BKA_GetWindowHandles"))) break;
		if (!((FARPROC&)RegisterCommand = GetProcAddress(m_hInstBecky, "BKA_RegisterCommand"))) break;
		if (!((FARPROC&)RegisterUICallback = GetProcAddress(m_hInstBecky, "BKA_RegisterUICallback"))) break;
		if (!((FARPROC&)GetDataFolder = GetProcAddress(m_hInstBecky, "BKA_GetDataFolder"))) break;
		if (!((FARPROC&)GetTempFolder = GetProcAddress(m_hInstBecky, "BKA_GetTempFolder"))) break;
		if (!((FARPROC&)GetTempFileName = GetProcAddress(m_hInstBecky, "BKA_GetTempFileName"))) break;
		if (!((FARPROC&)GetCurrentMailBox = GetProcAddress(m_hInstBecky, "BKA_GetCurrentMailBox"))) break;
		if (!((FARPROC&)SetCurrentMailBox = GetProcAddress(m_hInstBecky, "BKA_SetCurrentMailBox"))) break;
		if (!((FARPROC&)GetCurrentFolder = GetProcAddress(m_hInstBecky, "BKA_GetCurrentFolder"))) break;
		if (!((FARPROC&)SetCurrentFolder = GetProcAddress(m_hInstBecky, "BKA_SetCurrentFolder"))) break;
		if (!((FARPROC&)GetFolderDisplayName = GetProcAddress(m_hInstBecky, "BKA_GetFolderDisplayName"))) break;
		if (!((FARPROC&)SetMessageText = GetProcAddress(m_hInstBecky, "BKA_SetMessageText"))) break;
		if (!((FARPROC&)GetCurrentMail = GetProcAddress(m_hInstBecky, "BKA_GetCurrentMail"))) break;
		if (!((FARPROC&)SetCurrentMail = GetProcAddress(m_hInstBecky, "BKA_SetCurrentMail"))) break;
		if (!((FARPROC&)GetNextMail = GetProcAddress(m_hInstBecky, "BKA_GetNextMail"))) break;
		if (!((FARPROC&)SetSel = GetProcAddress(m_hInstBecky, "BKA_SetSel"))) break;
		if (!((FARPROC&)AppendMessage = GetProcAddress(m_hInstBecky, "BKA_AppendMessage"))) break;
		if (!((FARPROC&)MoveSelectedMessages = GetProcAddress(m_hInstBecky, "BKA_MoveSelectedMessages"))) break;
		if (!((FARPROC&)GetStatus = GetProcAddress(m_hInstBecky, "BKA_GetStatus"))) break;
		if (!((FARPROC&)ComposeMail = GetProcAddress(m_hInstBecky, "BKA_ComposeMail"))) break;
		if (!((FARPROC&)GetCharSet = GetProcAddress(m_hInstBecky, "BKA_GetCharSet"))) break;
		if (!((FARPROC&)GetSource = GetProcAddress(m_hInstBecky, "BKA_GetSource"))) break;
		if (!((FARPROC&)SetSource = GetProcAddress(m_hInstBecky, "BKA_SetSource"))) break;
		if (!((FARPROC&)GetHeader = GetProcAddress(m_hInstBecky, "BKA_GetHeader"))) break;
		if (!((FARPROC&)GetText = GetProcAddress(m_hInstBecky, "BKA_GetText"))) break;
		if (!((FARPROC&)SetText = GetProcAddress(m_hInstBecky, "BKA_SetText"))) break;
		if (!((FARPROC&)GetSpecifiedHeader = GetProcAddress(m_hInstBecky, "BKA_GetSpecifiedHeader"))) break;
		if (!((FARPROC&)SetSpecifiedHeader = GetProcAddress(m_hInstBecky, "BKA_SetSpecifiedHeader"))) break;
		if (!((FARPROC&)CompGetCharSet = GetProcAddress(m_hInstBecky, "BKA_CompGetCharSet"))) break;
		if (!((FARPROC&)CompGetSource = GetProcAddress(m_hInstBecky, "BKA_CompGetSource"))) break;
		if (!((FARPROC&)CompSetSource = GetProcAddress(m_hInstBecky, "BKA_CompSetSource"))) break;
		if (!((FARPROC&)CompGetHeader = GetProcAddress(m_hInstBecky, "BKA_CompGetHeader"))) break;
		if (!((FARPROC&)CompGetSpecifiedHeader = GetProcAddress(m_hInstBecky, "BKA_CompGetSpecifiedHeader"))) break;
		if (!((FARPROC&)CompSetSpecifiedHeader = GetProcAddress(m_hInstBecky, "BKA_CompSetSpecifiedHeader"))) break;
		if (!((FARPROC&)CompGetText = GetProcAddress(m_hInstBecky, "BKA_CompGetText"))) break;
		if (!((FARPROC&)CompSetText = GetProcAddress(m_hInstBecky, "BKA_CompSetText"))) break;
		if (!((FARPROC&)CompAttachFile = GetProcAddress(m_hInstBecky, "BKA_CompAttachFile"))) break;
		if (!((FARPROC&)Alloc = GetProcAddress(m_hInstBecky, "BKA_Alloc"))) break;
		if (!((FARPROC&)ReAlloc = GetProcAddress(m_hInstBecky, "BKA_ReAlloc"))) break;
		if (!((FARPROC&)Free = GetProcAddress(m_hInstBecky, "BKA_Free"))) break;
		if (!((FARPROC&)ISO_2022_JP = GetProcAddress(m_hInstBecky, "BKA_ISO_2022_JP"))) break;
		if (!((FARPROC&)ISO_2022_KR = GetProcAddress(m_hInstBecky, "BKA_ISO_2022_KR"))) break;
		if (!((FARPROC&)HZ_GB2312 = GetProcAddress(m_hInstBecky, "BKA_HZ_GB2312"))) break;
		if (!((FARPROC&)ISO_8859_2 = GetProcAddress(m_hInstBecky, "BKA_ISO_8859_2"))) break;
		if (!((FARPROC&)EUC_JP = GetProcAddress(m_hInstBecky, "BKA_EUC_JP"))) break;
		if (!((FARPROC&)UTF_7 = GetProcAddress(m_hInstBecky, "BKA_UTF_7"))) break;
		if (!((FARPROC&)UTF_8 = GetProcAddress(m_hInstBecky, "BKA_UTF_8"))) break;
		if (!((FARPROC&)B64Convert = GetProcAddress(m_hInstBecky, "BKA_B64Convert"))) break;
		if (!((FARPROC&)QPConvert = GetProcAddress(m_hInstBecky, "BKA_QPConvert"))) break;
		if (!((FARPROC&)MIMEHeader = GetProcAddress(m_hInstBecky, "BKA_MIMEHeader"))) break;
		if (!((FARPROC&)SerializeRcpts = GetProcAddress(m_hInstBecky, "BKA_SerializeRcpts"))) break;
		if (!((FARPROC&)Connect = GetProcAddress(m_hInstBecky, "BKA_Connect"))) break;

		// v2.00.06 or higher
		(FARPROC&)SetStatus = GetProcAddress(m_hInstBecky, "BKA_SetStatus");

		// v2.05 or higher
		(FARPROC&)NextUnread = GetProcAddress(m_hInstBecky, "BKA_NextUnread");

		// v2.40 or higher
		(FARPROC&)ProcessMail = GetProcAddress(m_hInstBecky, "BKA_ProcessMail");

		// v2.43 or higher
		(FARPROC&)GetSize = GetProcAddress(m_hInstBecky, "BKA_GetSize");

		// v2.50 or higher
		(FARPROC&)MoveMessages = GetProcAddress(m_hInstBecky, "BKA_MoveMessages");


		LPCTSTR lpVer = GetVersion();
		if (lstrlen(lpVer) > 7 ||
			lstrcmp(lpVer, "2.00.06") >= 0) {
			if (!SetStatus) {
				break;
			}
		} else {
			// Still works if the plugin doesn't use "SetStatus" API.
		}
		bRet = TRUE;
	} while (0);
	if (!bRet) {
		m_hInstBecky = NULL;
	}
	return bRet;
}
