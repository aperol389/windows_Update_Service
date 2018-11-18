
/*
*
* Author:	aperol
* Time:		2018-07-30
*
*/

#include "UtopiaWindowsUpdataDriver.h"

void outputEventLog(WORD logType, LPCWSTR logString)
{
	ReportEvent(gEventLog, logType, 0, gEventID++, NULL, 1, 0, &logString, NULL);
	return;
}

#define printEventLog(logType, fmt,...) \
{ \
wchar_t buffer[256]; \
std::swprintf(buffer, sizeof(buffer) / sizeof(*buffer), fmt, ## __VA_ARGS__);\
outputEventLog(logType, buffer); \
} \

eCode utopiaWindowsUpdateService::onStop()
{
	printEventLog(
		EVENTLOG_INFORMATION_TYPE,
		L"enter onStop function.");
	this->serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	SetServiceStatus(this->serviceStatusHandler, &this->serviceStatus);

	if (NULL != hDevNotify)
	{
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"unregister device notification.");
		UnregisterDeviceNotification(this->serviceStatusHandler);
	}

	this->serviceStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(this->serviceStatusHandler, &this->serviceStatus);

	return eCode_Success;
}

eCode utopiaWindowsUpdateService::registerStart()
{
	printEventLog(
		EVENTLOG_INFORMATION_TYPE,
		L"enter registerStart function.");
	this->serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(this->serviceStatusHandler, &this->serviceStatus);

	this->serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	this->serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(this->serviceStatusHandler, &this->serviceStatus);

	return eCode_Success;
}

DWORD WINAPI serviceControlHandlerEx(
	DWORD controlCode,
	DWORD dwEventType,
	LPVOID lpEventData,
	LPVOID lpContext)
{
	utopiaWindowsUpdateService *pService = (utopiaWindowsUpdateService *)lpContext;
	switch (controlCode)
	{
	case SERVICE_CONTROL_INTERROGATE:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_INTERROGATE.");
		break;

	case SERVICE_CONTROL_STOP:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_STOP.");
		if (eCode_Success == pService->onStop())
		{
			printEventLog(
				EVENTLOG_INFORMATION_TYPE,
				L"service control stop success.");
		}
		else
		{
			printEventLog(
				EVENTLOG_ERROR_TYPE,
				L"service control stop failed.");
		}
		break;

	case SERVICE_CONTROL_SHUTDOWN:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_SHUTDOWN.");
		if (eCode_Success == pService->onStop())
		{
			printEventLog(
				EVENTLOG_INFORMATION_TYPE,
				L"service control stop success.");
		}
		else
		{
			printEventLog(
				EVENTLOG_INFORMATION_TYPE,
				L"service control stop failed.");
		}
		break;
	
	case SERVICE_CONTROL_PAUSE:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_PAUSE.");
		break;

	case SERVICE_CONTROL_CONTINUE:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_CONTINUE.");
		break;

	case SERVICE_CONTROL_DEVICEEVENT:
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"control code: SERVICE_CONTROL_DEVICEEVENT.");
		if (DBT_DEVICEARRIVAL == dwEventType)
		{
			printEventLog(
				EVENTLOG_INFORMATION_TYPE,
				L"device event: device arrival, waiting for 5 seconds...");
			Sleep(5000);

			DEV_BROADCAST_DEVICEINTERFACE *pNotificationFilter = (DEV_BROADCAST_DEVICEINTERFACE *)lpEventData;
			GUID InterfaceClassGUID = pNotificationFilter->dbcc_classguid;

			size_t length = wcslen(pNotificationFilter->dbcc_name);
			memcpy(pService->devName, pNotificationFilter->dbcc_name, length * sizeof(wchar_t));
			
			HKEY hKey = NULL;
			DWORD dwSize = MAX_PATH;
			LSTATUS status = ERROR_SUCCESS;

			// if (wcsstr(pService->devName, UTOPIA_PID) != 0)
			if (wcsstr(pService->devName, SLASH_PID) != 0)
			{
				printEventLog(
					EVENTLOG_WARNING_TYPE,
					L"arrival device is utopia.");
			}
			else
			{
				printEventLog(
					EVENTLOG_ERROR_TYPE,
					L"arrival device is not utopia.");
			}
		}
		break;
	}
	return 0;
}

eCode utopiaWindowsUpdateService::initService()
{
	printEventLog(
		EVENTLOG_INFORMATION_TYPE,
		L"enter initialize service function.");

	this->serviceStatus = { 0 };
	this->serviceStatus.dwServiceType = SERVICE_WIN32;
	this->serviceStatus.dwCurrentState = SERVICE_STOPPED;
	this->serviceStatus.dwControlsAccepted = 0;
	this->serviceStatus.dwWin32ExitCode = NO_ERROR;
	this->serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	this->serviceStatus.dwCheckPoint = 0;
	this->serviceStatus.dwWaitHint = 0;

	this->serviceStatusHandler = RegisterServiceCtrlHandlerEx(
		SERVICE_NAME,
		serviceControlHandlerEx,
		this);
	if (NULL == this->serviceStatusHandler)
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"register service control handler failed.");
		return eCode_Error;
	}
	else
	{
		printEventLog(
			EVENTLOG_INFORMATION_TYPE,
			L"register service control handler success.");
		this->registerStart();
	}

	DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
	notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	notificationFilter.dbcc_classguid = INTERFACE_CLASS;

	this->hDevNotify = RegisterDeviceNotification(
		this->serviceStatusHandler,
		&notificationFilter,
		DEVICE_NOTIFY_SERVICE_HANDLE);
	if (NULL == this->hDevNotify)
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"register device notification failed.");
		return eCode_Error;
	}
	else
	{
		printEventLog(
			EVENTLOG_SUCCESS,
			L"register device notification success.");
		return eCode_Success;
	}
}

void WINAPI serviceMain(DWORD argc, LPWSTR *argv)
{
	printEventLog(
		EVENTLOG_INFORMATION_TYPE,
		L"enter service main function.");
	utopiaWindowsUpdateService *pService = new utopiaWindowsUpdateService;

	if (eCode_Success == pService->initService())
	{
		printEventLog(
			EVENTLOG_SUCCESS,
			L"initialize service success.");
		return;
	}
	else
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"initialize service failed.");
		return;
	}
}

/*
* uninstall utopia windows update service.
*/
eCode uninstallService()
{
	eCode errorCode = eCode_Init;

	SC_HANDLE serviceControlManager = NULL;
	serviceControlManager = OpenSCManager(
		0,
		0,
		SC_MANAGER_CONNECT);
	if (!serviceControlManager)
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"open service control manager failed.");
		errorCode = eCode_Error;
		return errorCode;
	}

	SC_HANDLE service = NULL;
	service = OpenService(
		serviceControlManager,
		SERVICE_NAME,
		SERVICE_QUERY_STATUS | DELETE);
	if (!service)
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"Open utopia windows update service failed.");
		CloseServiceHandle(serviceControlManager);
		errorCode = eCode_Error;
		return errorCode;
	}
	else
	{
		SERVICE_STATUS serviceStatus;
		if (QueryServiceStatus(service, &serviceStatus))
		{
			if (SERVICE_STOPPED == serviceStatus.dwCurrentState)
			{
				printEventLog(
					EVENTLOG_WARNING_TYPE,
					L"service stopped.");
				if (!DeleteService(service))
				{
					printEventLog(
						EVENTLOG_ERROR_TYPE,
						L"service delete failed.");
					errorCode = eCode_Error;
				}
				else
				{
					printEventLog(
						EVENTLOG_SUCCESS,
						L"service delete success.");
					errorCode = eCode_Success;
				}
			}
			else
			{
				printEventLog(
					EVENTLOG_ERROR_TYPE,
					L"service not stopped, can not delete this service.");
				errorCode = eCode_Error;
			}
		}
		else
		{
			printEventLog(
				EVENTLOG_ERROR_TYPE,
				L"query service status failed.");
			errorCode = eCode_Error;
		}

		CloseServiceHandle(service);
		CloseServiceHandle(serviceControlManager);
		return errorCode;
	}
}

/*
* install utopia windows update service.
*/
eCode installService()
{
	eCode errorCode = eCode_Init;
	SC_HANDLE serviceControlManager = NULL;
	serviceControlManager = OpenSCManager(
		0,
		0,
		SC_MANAGER_CREATE_SERVICE);

	if (!serviceControlManager)
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"open service control manager failed.");
		errorCode = eCode_Error;
		return errorCode;
	}

	WCHAR path[MAX_PATH];
	if (GetModuleFileName(0, path, MAX_PATH) > 0)
	{
		SC_HANDLE service = NULL;
		service = CreateService(
			serviceControlManager,
			SERVICE_NAME,
			DISPLAY_NAME,
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_CRITICAL,
			path,
			0,
			0,
			0,
			0,
			0);
		if (!service)
		{
			printEventLog(
				EVENTLOG_ERROR_TYPE,
				L"create service failed.");
			CloseServiceHandle(serviceControlManager);
			errorCode = eCode_Error;
		}
		else
		{
			printEventLog(
				EVENTLOG_SUCCESS,
				L"create service success.");
			CloseServiceHandle(service);
			CloseServiceHandle(serviceControlManager);
			errorCode = eCode_Success;;
		}
	}
	else
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"get module file name failed.");
		CloseServiceHandle(serviceControlManager);
		errorCode = eCode_Error;
	}

	return errorCode;
}

void runService()
{
	SERVICE_TABLE_ENTRY serviceTable[] = {
		{ SERVICE_NAME, serviceMain },
		{ 0, 0 }
	};

	// Sleep(15000);
	if (!StartServiceCtrlDispatcher(serviceTable))
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"start service control dispatcher failed, GetLastErrorCode: %d", GetLastError());
		return;
	}

	printEventLog(
		EVENTLOG_SUCCESS,
		L"run service success");

	return;
}

int main(int argc, char **argv)
{
	if (1 == argc)
	{
		printEventLog(
			EVENTLOG_WARNING_TYPE,
			L"run utopia windows update normally.");
		runService();
		return 0;
	}
	else if (2 == argc)
	{
		if (!strcmp(argv[1], "install"))
		{
			printEventLog(
				EVENTLOG_WARNING_TYPE,
				L"service start install.");
			if (eCode_Success == installService())
			{
				printEventLog(
					EVENTLOG_SUCCESS,
					L"service install success.");
				return 0;
			}
			else
			{
				printEventLog(
					EVENTLOG_ERROR_TYPE,
					L"service install failed.");
				return -1;
			}
		}
		else if(!strcmp(argv[1], "uninstall"))
		{
			if (eCode_Success == uninstallService())
			{
				printEventLog(
					EVENTLOG_SUCCESS,
					L"service uninstall success.");
				return 0;
			}
			else
			{
				printEventLog(
					EVENTLOG_ERROR_TYPE,
					L"service uninstall failed.");
				return -1;
			}
		}
		else
		{
			printEventLog(
				EVENTLOG_ERROR_TYPE,
				L"service start with bad parameters.");
			return -1;
		}
	}
	else
	{
		printEventLog(
			EVENTLOG_ERROR_TYPE,
			L"service start with bad parameters.");
		return -1;
	}
}