///////////////////////////////////////////////////////////////////////////
// Becky! API header file
//
// You can modify and redistribute this file without any permission.

#ifndef _BECKY_API
#define _BECKY_API

#define BKC_MENU_MAIN		0
#define BKC_MENU_LISTVIEW	1
#define BKC_MENU_TREEVIEW	2
#define BKC_MENU_MSGVIEW	3
#define BKC_MENU_MSGEDIT	4
#define BKC_MENU_TASKTRAY	5
#define BKC_MENU_COMPOSE	10
#define BKC_MENU_COMPEDIT	11
#define BKC_MENU_COMPREF	12

#define BKC_BITMAP_ADDRESSBOOKICON	1
#define BKC_BITMAP_ADDRESSPERSON	2
#define BKC_BITMAP_ANIMATION		3
#define BKC_BITMAP_FOLDERCOLOR		4
#define BKC_BITMAP_FOLDERICON		5
#define BKC_BITMAP_LISTICON			6
#define BKC_BITMAP_PRIORITYSTAMP	7
#define BKC_BITMAP_RULETREEICON		8
#define BKC_BITMAP_TEMPLATEFOLDER	9
#define BKC_BITMAP_WHATSNEWLIST		10
#define BKC_BITMAP_LISTICON2		11
#define BKC_BITMAP_AGENTS			12

#define BKC_ICON_ADDRESSBOOK		101
#define BKC_ICON_ANIMATION1_SMALL	102
#define BKC_ICON_ANIMATION2_SMALL	103
#define	BKC_ICON_COMPOSEFRAME		104
#define BKC_ICON_MAINFRAME			105
#define BKC_ICON_NEWARRIVAL1_SMALL	106		
#define BKC_ICON_NEWARRIVAL2_SMALL	107

#define BKC_TOOLBAR_ADDRESSBOOK		201
#define BKC_TOOLBAR_COMPOSEFRAME	202
#define BKC_TOOLBAR_HTMLEDITOR		203
#define BKC_TOOLBAR_MAINFRAME		204

#define BKC_ONSEND_ERROR	-1
#define BKC_ONSEND_PROCESSED	-2

#define BKC_FILTER_DEFAULT	0
#define BKC_FILTER_PASS		1
#define BKC_FILTER_DONE		2
#define BKC_FILTER_NEXT		3

#define ACTION_NOTHING		-1
#define ACTION_MOVEFOLDER	0
#define ACTION_COLORLABEL	1
#define ACTION_SETFLAG		2
#define ACTION_SOUND		3
#define ACTION_RUNEXE		4
#define ACTION_REPLY		5
#define ACTION_FORWARD		6
#define ACTION_LEAVESERVER	7
#define ACTION_ADDHEADER	8

#define MESSAGE_READ		 0x00000001
#define MESSAGE_FORWARDED	 0x00000002
#define MESSAGE_REPLIED		 0x00000004
#define MESSAGE_ATTACHMENT	 0x00000008
#define MESSAGE_PARTIAL		 0x00000100 
#define MESSAGE_REDIRECT	 0x00000200

#define COMPOSE_MODE_COMPOSE1	0
#define COMPOSE_MODE_COMPOSE2	1
#define COMPOSE_MODE_COMPOSE3	2
#define COMPOSE_MODE_TEMPLATE	3
#define COMPOSE_MODE_REPLY1		5
#define COMPOSE_MODE_REPLY2		6
#define COMPOSE_MODE_REPLY3		7
#define COMPOSE_MODE_FORWARD1	10
#define COMPOSE_MODE_FORWARD2	11
#define COMPOSE_MODE_FORWARD3	12

#define BKMENU_CMDUI_DISABLED		1
#define BKMENU_CMDUI_CHECKED		2

class CBeckyAPI
{
public:
	CBeckyAPI()
	{
		m_hInstBecky = NULL;
	}
	~CBeckyAPI()
	{
		if (m_hInstBecky) {
			//::FreeLibrary(m_hInstBecky);
		}
	}
	BOOL InitAPI();

	LPCTSTR (WINAPI* GetVersion)();
	void    (WINAPI* Command)(HWND hWnd, LPCTSTR lpCmd);
	BOOL	(WINAPI* GetWindowHandles)(HWND* lphMain, HWND* lphTree, HWND* lphList, HWND* lphView);
	UINT	(WINAPI* RegisterCommand)(LPCTSTR lpszComment, int nTarget, void (CALLBACK* lpCallback)(HWND, LPARAM));
	UINT	(WINAPI* RegisterUICallback)(UINT nID, UINT (CALLBACK* lpCallback)(HWND, LPARAM));
	LPCTSTR (WINAPI* GetDataFolder)();
	LPCTSTR (WINAPI* GetTempFolder)();
	LPCTSTR (WINAPI* GetTempFileName)(LPCTSTR lpType);
	LPCTSTR (WINAPI* GetCurrentMailBox)();
	void	(WINAPI* SetCurrentMailBox)(LPCTSTR lpMailBox);
	LPCTSTR (WINAPI* GetCurrentFolder)();
	void	(WINAPI* SetCurrentFolder)(LPCTSTR lpFolderID);
	LPCTSTR (WINAPI* GetFolderDisplayName)(LPCSTR lpFolderID);
	void	(WINAPI* SetMessageText)(HWND hWnd, LPCSTR lpszMsg);
	LPCTSTR (WINAPI* GetCurrentMail)();
	void	(WINAPI* SetCurrentMail)(LPCTSTR lpMailID);
	int		(WINAPI* GetNextMail)(int nStart, LPSTR lpszMailID, int nBuf, BOOL bSelected);
	void	(WINAPI* SetSel)(LPCTSTR lpMailID, BOOL bSel);
	BOOL	(WINAPI* AppendMessage)(LPCTSTR lpFolderID, LPCTSTR lpszData);
	int		(WINAPI* MoveMessages)(LPCTSTR lpFolderID, LPCTSTR lpMailIDSet, BOOL bCopy);
	BOOL	(WINAPI* MoveSelectedMessages)(LPCTSTR lpFolderID, BOOL bCopy);
	DWORD	(WINAPI* GetStatus)(LPCTSTR lpMailID);
	DWORD	(WINAPI* SetStatus)(LPCTSTR lpMailID, DWORD dwSet, DWORD dwReset);
	HWND	(WINAPI* ComposeMail)(LPCTSTR lpURL);
	int		(WINAPI* GetCharSet)(LPCTSTR lpMailID, LPSTR lpszCharSet, int nBuf);
	LPSTR	(WINAPI* GetSource)(LPCTSTR lpMailID);
	void	(WINAPI* SetSource)(LPCTSTR lpMailID, LPCTSTR lpSource);
	DWORD	(WINAPI* GetSize)(LPCTSTR lpMailID);
	LPSTR	(WINAPI* GetHeader)(LPCTSTR lpMailID);
	LPSTR	(WINAPI* GetText)(LPSTR lpszMimeType, int nBuf);
	void	(WINAPI* SetText)(int nMode, LPCTSTR lpText);
	void	(WINAPI* GetSpecifiedHeader)(LPCTSTR lpHeader, LPSTR lpszData, int nBuf);
	void	(WINAPI* SetSpecifiedHeader)(LPCTSTR lpHeader, LPCTSTR lpszData);
	int		(WINAPI* CompGetCharSet)(HWND hWnd, LPSTR lpszCharSet, int nBuf);
	LPSTR	(WINAPI* CompGetSource)(HWND hWnd);
	void	(WINAPI* CompSetSource)(HWND hWnd, LPCTSTR lpSource);
	LPSTR	(WINAPI* CompGetHeader)(HWND hWnd);
	void	(WINAPI* CompGetSpecifiedHeader)(HWND hWnd, LPCTSTR lpHeader, LPSTR lpszData, int nBuf);
	void	(WINAPI* CompSetSpecifiedHeader)(HWND hWnd, LPCTSTR lpHeader, LPCTSTR lpszData);
	LPSTR	(WINAPI* CompGetText)(HWND hWnd, LPSTR lpszMimeType, int nBuf);
	void	(WINAPI* CompSetText)(HWND hWnd, int nMode, LPCTSTR lpText);
	void	(WINAPI* CompAttachFile)(HWND hWnd, LPCTSTR lpAttachFile, LPCTSTR lpMimeType);
	LPVOID	(WINAPI* Alloc)(DWORD dwSize);
	LPVOID	(WINAPI* ReAlloc)(LPVOID lpVoid, DWORD dwSize);
	void	(WINAPI* Free)(LPVOID lpVoid);
	LPSTR	(WINAPI* ISO_2022_JP)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* ISO_2022_KR)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* HZ_GB2312)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* ISO_8859_2)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* EUC_JP)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* UTF_7)(LPCTSTR lpSrc, BOOL bEncode);
	LPSTR	(WINAPI* UTF_8)(LPCTSTR lpSrc, BOOL bEncode);
	BOOL	(WINAPI* B64Convert)(LPCTSTR lpszOutFile, LPCTSTR lpszInFile, BOOL bEncode);
	BOOL	(WINAPI* QPConvert)(LPCTSTR lpszOutFile, LPCTSTR lpszInFile, BOOL bEncode);
	LPSTR	(WINAPI* MIMEHeader)(LPCTSTR lpszIn, LPSTR lpszCharSet, int nBuf, BOOL bEncode);
	LPSTR	(WINAPI* SerializeRcpts)(LPCTSTR lpAddresses);
	BOOL	(WINAPI* Connect)(BOOL bConnect);
	BOOL	(WINAPI* NextUnread)(BOOL bBackScroll, BOOL bGoNext);
	void	(WINAPI* ProcessMail)(LPCTSTR lpMailID, int nAction, LPCTSTR lpParam);
protected:
	HINSTANCE m_hInstBecky;
};

#endif