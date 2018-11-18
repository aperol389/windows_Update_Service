
#pragma once

/*
*
* Description:
*	This program is used to implement utopia windows update.
*
* Author:	aperol
* Time:		2018-07-30
*
*/

#include "windows.h"
#include "string"
#include "Dbt.h"

using namespace std;

HANDLE gEventLog = RegisterEventSource(NULL, L"PolycomWindowsUpdateService");
DWORD gEventID = 1;

#define INTERFACE_CLASS {0xAD87D424,0x938D,0x4A61,{0xB0,0x6B,0x23,0x71,0x36,0x3E,0xFC,0x31}}
#define SERVICE_NAME L"PolycomWindowsUpdateService"
#define DISPLAY_NAME L"PolycomWindowsUpdateService"
#define UTOPIA_PID L"PID_921E"
#define SLASH_PID L"PID_3001"

enum eCode {
	eCode_Success = 0,
	eCode_Error,
	eCode_Init
};

class utopiaWindowsUpdateService
{
private:
	SERVICE_STATUS serviceStatus;
	SERVICE_STATUS_HANDLE serviceStatusHandler;
	HDEVNOTIFY hDevNotify;
	wchar_t devName[MAX_PATH];

	eCode errorCode;

	eCode onStop();
	eCode registerStart();
	eCode initService();

	friend DWORD WINAPI serviceControlHandlerEx(
		DWORD controlCode,
		DWORD dwEventType,
		LPVOID lpEventData,
		LPVOID lpContext);

	friend void WINAPI serviceMain(
		DWORD argc,
		LPWSTR *argv);

	friend DWORD WINAPI copyFileToDeviceThread(
		LPVOID parameter);
};
