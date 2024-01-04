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
    OutputDebugString(L"Plugin started");
    return 0;
}


extern "C" int WINAPI BKC_OnExit(){
    OutputDebugString(L"Becky closing");
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        return B2_API.InitAPI();
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
