# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <tchar.h>
# include <tlhelp32.h>
#pragma comment(lib, "advapi32.lib")
#pragma warning(disable:4191 4706)
#define SVCNAME TEXT("tinky")
#define WINKEY TEXT("winkey.exe")

SC_HANDLE scManager;

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = nullptr;

void WINAPI ServiceMain();
void WINAPI ServiceInit();
void WINAPI ServiceControlHandler(DWORD dwCtrl);
void ServiceReportStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint);
HANDLE ServiceGetToken();

void ServiceStartProcess(STARTUPINFO* si,
	PROCESS_INFORMATION* pi);

int usage(int ac, char** av)
{
	if (ac == 2 && (strcmp(av[1], "install")
		&& strcmp(av[1], "start")
		&& strcmp(av[1], "stop")
		&& strcmp(av[1], "disable")
		&& strcmp(av[1], "enable")
		&& strcmp(av[1], "update")
		&& strcmp(av[1], "query")
		&& strcmp(av[1], "delete")))
	{
		printf("Usage: %s [install | start | stop | delete | query | disable | enable | update ]\n", av[0]);
		return(0);
	}
	else if (ac == 2)
	{
		if (!strcmp(av[1], "install"))
		{
			printf("install \n");
			return (1);
		}
		else if (!strcmp(av[1], "start"))
		{
			printf("start\n");
			return (2);
		}
		else if (!strcmp(av[1], "stop"))
		{
			printf("stop\n");
			return (3);
		}
		else if (!strcmp(av[1], "delete"))
		{
			printf("delete\n");
			return (4);
		}
		else if (!strcmp(av[1], "update"))
		{
			printf("update\n");
			return (5);
		}
		else if (!strcmp(av[1], "query"))
		{
			printf("query\n");
			return (6);
		}
		else if (!strcmp(av[1], "enable"))
		{
			printf("enable\n");
			return (7);
		}
		else if (!strcmp(av[1], "disable"))
		{
			printf("disable\n");
			return (8);
		}
	}
	return (0);
}

int assign_scManager()
{
	unsigned int err;

	scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager)
	{
		err = GetLastError();
		if (err == ERROR_ACCESS_DENIED)
			printf("No Admin rights \n");
		else
			printf("OpenscManager failed error (%ld)\n", GetLastError());
		return (1);
	}
	return (0);
}

// Create and install service tinky --- 

int install_sc(void)
{
	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (scService)
	{
		printf("Service %s exists.\n", SVCNAME);
		return (1);
	}
	scService = CreateService(scManager, SVCNAME, SVCNAME,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
		"C:\\Users\\IEUser\\Desktop\\svc.exe", NULL, NULL, NULL, NULL, NULL);
	if (!scService)
	{
		printf("CreateService failed error (%ld)\n", GetLastError());
		return (1);
	}
	printf("Service %s installed successfully.\n", SVCNAME);
	return (0);
}

//Start service // need dispatcher before start : https://stackoverflow.com/questions/1640114/startservice-fails-with-error-code-1053

int start_sc(void)
{
	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (scService)
	{
		if (!StartService(scService, 0, NULL))
		{
			DWORD err = GetLastError();
			if (err == ERROR_SERVICE_ALREADY_RUNNING)
				printf("Service %s already running.\n", SVCNAME);
			else
				printf("StartService failed error (%ld)\n", err);
			if (!CloseServiceHandle(scService))
				printf("CloseServiceHandle failed error (%ld)\n", err);
			return (1);
		}
		printf("Service %s started successfully.\n", SVCNAME);
		if (!CloseServiceHandle(scService))
			printf("CloseServiceHandle failed error (%ld)\n", GetLastError());
	}
	else
		printf("Service %s is not installed.\n", SVCNAME);
	return (0);
}

void stop_sc()
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwBytesNeeded;
	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000;
	DWORD dwWaitTime;

	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (scService == nullptr) {
		return;
	}
	/* Check if service is started */
	if (!QueryServiceStatusEx(
		scService,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded
	)) {
		printf("QueryServiceStatusEx failed (%ld)\n", GetLastError());
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED) {
		printf("Service is already stopped\n");
		goto stop_cleanup;
	}

	/* Wait for stoping service if pendind*/
	while (ssp.dwCurrentState == SERVICE_STOP_PENDING) {
		printf("Service stop pending...\n");
		dwWaitTime = ssp.dwWaitHint / 10;
		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;
		Sleep(dwWaitTime);
		if (!QueryServiceStatusEx(scService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded
		)) {
			printf("QueryServiceStatusEx failed %ld\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED) {
			printf("Service stopped successfully.\n");
			goto stop_cleanup;
		}

		if (GetTickCount() - dwStartTime > dwTimeout) {
			printf("Service stop timed out.\n");
			goto stop_cleanup;
		}
	}
	// send stop code
	if (!ControlService(
		scService,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS)&ssp
	)) {
		printf("ControlService failed (%ld)\n", GetLastError());
		goto stop_cleanup;
	}
	/* Wait service to stop */
	while (ssp.dwCurrentState != SERVICE_STOPPED) {
		Sleep(ssp.dwWaitHint);
		if (!QueryServiceStatusEx(scService,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded
		)) {
			printf("QueryServiceStatusEx failed (%ld)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount() - dwStartTime > dwTimeout) {
			printf("Wait timed out\n");
			goto stop_cleanup;
		}
	}
	printf("Service stopped successfully\n");

stop_cleanup:
	CloseServiceHandle(scService);
}

int delete_sc(void)
{
	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (!scService)
	{
		printf("Service %s is not installed.\n", SVCNAME);
		return (1);
	}
	SERVICE_STATUS	status;

	if (QueryServiceStatus(scService, &status))
	{
		if (status.dwCurrentState != SERVICE_STOPPED)
			printf("Service %s not stopped yet\n", SVCNAME);
		else
		{
			if (!DeleteService(scService))
				printf("DeleteService failed error (%ld)\n", GetLastError());
			else
			{
				printf("Service %s deleted successfully.\n", SVCNAME);
				return (0);
			}
		}
	}
	else
		printf("ControlService failed error (%ld)\n", GetLastError());
	if (!CloseServiceHandle(scService))
		printf("CloseServiceHandle failed error (%ld)\n", GetLastError());
	return (1);
}

VOID __stdcall DoQuerySvc()
{
	SC_HANDLE schService;
	LPQUERY_SERVICE_CONFIG lpsc = 0;
	LPSERVICE_DESCRIPTION lpsd = 0;
	DWORD dwBytesNeeded, dwError, cbBufSize = 0;

	// Get a handle to the service.

	schService = OpenService(
		scManager,          // SCM database 
		SVCNAME,             // name of service 
		SERVICE_QUERY_CONFIG); // need query config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%ld)\n", GetLastError());
		return;
	}

	// Get the configuration information.

	if (!QueryServiceConfig(
		schService,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsc = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig failed (%ld)", dwError);
			goto cleanup;
		}
	}

	if (!QueryServiceConfig(
		schService,
		lpsc,
		cbBufSize,
		&dwBytesNeeded))
	{
		printf("QueryServiceConfig failed (%ld)", GetLastError());
		goto cleanup;
	}

	if (!QueryServiceConfig2(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		NULL,
		0,
		&dwBytesNeeded))
	{
		dwError = GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwError)
		{
			cbBufSize = dwBytesNeeded;
			lpsd = (LPSERVICE_DESCRIPTION)LocalAlloc(LMEM_FIXED, cbBufSize);
		}
		else
		{
			printf("QueryServiceConfig2 failed (%ld)", dwError);
			goto cleanup;
		}
	}

	if (!QueryServiceConfig2(
		schService,
		SERVICE_CONFIG_DESCRIPTION,
		(LPBYTE)lpsd,
		cbBufSize,
		&dwBytesNeeded))
	{
		printf("QueryServiceConfig2 failed (%ld)", GetLastError());
		goto cleanup;
	}

	// Print the configuration information.

	_tprintf(TEXT("%s configuration: \n"), SVCNAME);
	_tprintf(TEXT("  Type: 0x%lx\n"), lpsc->dwServiceType);
	_tprintf(TEXT("  Start Type: 0x%lx\n"), lpsc->dwStartType);
	_tprintf(TEXT("  Error Control: 0x%lx\n"), lpsc->dwErrorControl);
	_tprintf(TEXT("  Binary path: %s\n"), lpsc->lpBinaryPathName);
	_tprintf(TEXT("  Account: %s\n"), lpsc->lpServiceStartName);

	if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
		_tprintf(TEXT("  Description: %s\n"), lpsd->lpDescription);
	if (lpsc->lpLoadOrderGroup != NULL && lstrcmp(lpsc->lpLoadOrderGroup, TEXT("")) != 0)
		_tprintf(TEXT("  Load order group: %s\n"), lpsc->lpLoadOrderGroup);
	if (lpsc->dwTagId != 0)
		_tprintf(TEXT("  Tag ID: %ld\n"), lpsc->dwTagId);
	if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
		_tprintf(TEXT("  Dependencies: %s\n"), lpsc->lpDependencies);

	LocalFree(lpsc);
	LocalFree(lpsd);

cleanup:
	CloseServiceHandle(schService);
}


VOID __stdcall DoDisableSvc()
{
	SC_HANDLE schService;

	schService = OpenService(
		scManager,            // SCM database 
		SVCNAME,               // name of service 
		SERVICE_CHANGE_CONFIG);  // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%ld)\n", GetLastError());
		CloseServiceHandle(scManager);
		return;
	}

	if (!ChangeServiceConfig(
		schService,        // handle of service 
		SERVICE_NO_CHANGE, // service type: no change 
		SERVICE_DISABLED,  // service start type 
		SERVICE_NO_CHANGE, // error control: no change 
		NULL,              // binary path: no change 
		NULL,              // load order group: no change 
		NULL,              // tag ID: no change 
		NULL,              // dependencies: no change 
		NULL,              // account name: no change 
		NULL,              // password: no change 
		NULL))            // display name: no change
	{
		printf("ChangeServiceConfig failed (%ld)\n", GetLastError());
	}
	else printf("Service disabled successfully.\n");

	CloseServiceHandle(schService);
}

VOID __stdcall DoEnableSvc()
{
	SC_HANDLE schService;

	schService = OpenService(
		scManager,            // SCM database 
		SVCNAME,               // name of service 
		SERVICE_CHANGE_CONFIG);  // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%ld)\n", GetLastError());
		CloseServiceHandle(scManager);
		return;
	}

	if (!ChangeServiceConfig(
		schService,            // handle of service 
		SERVICE_NO_CHANGE,     // service type: no change 
		SERVICE_DEMAND_START,  // service start type 
		SERVICE_NO_CHANGE,     // error control: no change 
		NULL,                  // binary path: no change 
		NULL,                  // load order group: no change 
		NULL,                  // tag ID: no change 
		NULL,                  // dependencies: no change 
		NULL,                  // account name: no change 
		NULL,                  // password: no change 
		NULL))                // display name: no change
	{
		printf("ChangeServiceConfig failed (%ld)\n", GetLastError());
	}
	else printf("Service enabled successfully.\n");

	CloseServiceHandle(schService);
}

VOID __stdcall DoUpdateSvcDesc()
{
	SC_HANDLE schService;
	SERVICE_DESCRIPTION sd;
	LPTSTR szDesc = TEXT("Description update of service");

	schService = OpenService(
		scManager,            // SCM database 
		SVCNAME,               // name of service 
		SERVICE_CHANGE_CONFIG);  // need change config access 

	if (schService == NULL)
	{
		printf("OpenService failed (%ld)\n", GetLastError());
		CloseServiceHandle(scManager);
		return;
	}

	sd.lpDescription = szDesc;

	if (!ChangeServiceConfig2(
		schService,                 // handle to service
		SERVICE_CONFIG_DESCRIPTION, // change: description
		&sd))                      // new description
	{
		printf("ChangeServiceConfig2 failed\n");
	}
	else printf("Service description updated successfully.\n");

	CloseServiceHandle(schService);
}

void do_op(int op)
{
	if (op == 1)
		install_sc();
	else if (op == 2)
		start_sc();
	else if (op == 3)
		stop_sc();
	else if (op == 4)
	{
		stop_sc();
		delete_sc();
	}
	else if (op == 5)
		DoQuerySvc();
	else if (op == 6)
		DoUpdateSvcDesc();
	else if (op == 7)
		DoEnableSvc();
	else if (op == 8)
		DoDisableSvc();
}


void WINAPI ServiceMain()
{
	/* Save handle to edit the status later */
	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		reinterpret_cast<LPHANDLER_FUNCTION>(ServiceControlHandler)
	);

	if (!gSvcStatusHandle) {
		printf("RegisterServiceCtrlHandler failed\n");
		return;
	}

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;
	/* Set service to Pending */
	ServiceReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	/* Init service and continue flow */
	ServiceInit();
}

void WINAPI ServiceInit()
{
	/* Create event */
	ghSvcStopEvent = CreateEvent(
		nullptr,
		TRUE,
		FALSE,
		nullptr
	);
	if (ghSvcStopEvent == nullptr) {
		ServiceReportStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}
	/* Set stat of running service */
	ServiceReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	/*Create proc to handle winkey */
	ServiceStartProcess(&si, &pi);
	/*Witing service to send stop request */
	while (1) {
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		ServiceReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
		break;
	}
	/* Force terminate process */
	if (pi.hProcess) {
		TerminateProcess(pi.hProcess, 0);
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdError);
	CloseHandle(si.hStdOutput);
}

void WINAPI ServiceControlHandler(DWORD dwCtrl)
{
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		ServiceReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		SetEvent(ghSvcStopEvent);
		ServiceReportStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

void ServiceReportStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING) {
		gSvcStatus.dwControlsAccepted = 0;
	}
	else {
		gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
		gSvcStatus.dwCheckPoint = 0;
	}
	else {
		gSvcStatus.dwCheckPoint = ++dwCheckPoint;
	}
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

HANDLE ServiceGetToken()
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		printf("CreateToolhelp32Snapshot failed (%ld)\n", GetLastError());
		return nullptr;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32)) {
		printf("Process32First failed (%ld)\n", GetLastError());
		CloseHandle(hProcessSnap);
		return nullptr;
	}

	DWORD logonPID = 0;
	do {
		if (lstrcmpi(pe32.szExeFile, TEXT("winlogon.exe")) == 0) {
			logonPID = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);
	if (logonPID == 0) {
		printf("Process32Next failed (%ld)\n", GetLastError());
		return nullptr;
	}
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, logonPID);
	if (hProcess == nullptr) {
		printf("OpenProcess failed (%ld)\n", GetLastError());
		return nullptr;
	}
	HANDLE hToken = nullptr;
	OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);
	CloseHandle(hProcess);
	if (hToken == nullptr) {
		printf("OpenProcessToken failed (%ld)\n", GetLastError());
		return nullptr;
	}

	/* Duplicate token using Impersonation */
	HANDLE hTokenD = nullptr;
	DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenImpersonation, &hTokenD);
	CloseHandle(hToken);
	if (hTokenD == nullptr) {
		printf("DuplicateTokenEx failed (%ld)\n", GetLastError());
		return nullptr;
	}

	return hTokenD;
}

void ServiceStartProcess(STARTUPINFO* si, PROCESS_INFORMATION* pi)
{
	/* Fetch impersonated token */
	HANDLE hToken = ServiceGetToken();
	if (hToken == nullptr) {
		printf("GetToken failed (%ld)\n", GetLastError());
		return;
	}

	/* Open the winkey process */
	CreateProcessAsUser(hToken,
		TEXT("C:\\Users\\IEUser\\Desktop\\winkey.exe"),
		nullptr,
		nullptr,
		nullptr,
		false,
		NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
		NULL,
		NULL,
		si,
		pi
	);
	CloseHandle(hToken);
}
int main(int ac, char** av)
{
	int			op;

	if ((op = usage(ac, av)))
	{
		if (assign_scManager())
			return (1);
		do_op(op);
		if (!CloseServiceHandle(scManager))
		{
			printf("CloseServiceHandle failed error (%ld)\n", GetLastError());
			return (1);
		}
	}
	else {
		SERVICE_TABLE_ENTRY DispatchTable[] = { {
			const_cast<LPSTR>(SVCNAME),
			reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain)},
		{NULL, NULL}
		};
		StartServiceCtrlDispatcher(DispatchTable);
	}
	return 0;
}
