#include <string.h>
# include <windows.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <tlhelp32.h>
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("tinky")
#define WINKEY TEXT("winkey.exe")
SC_HANDLE scManager;


int usage(int ac, char **av)
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

int install_sc(char *path)
{
	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (scService)
	{
		printf("Service %s exists.\n", SVCNAME);
		return (1);
	}
	scService = CreateService(scManager,SVCNAME, SVCNAME,
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

int start_sc(char *path)
{
	SC_HANDLE scService = OpenService(scManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (scService)
	{
		const char *args[] = { path };
		if (!StartService(scService, 1, args))
		{
			DWORD err = GetLastError();
			if (err == ERROR_SERVICE_ALREADY_RUNNING)
				printf("Service %s already running.\n", SVCNAME);
			else
				printf("StartService failed error (%ld)\n", err);
			if (!CloseServiceHandle(scService))
				printf("CloseServiceHandle failed error (%ld)\n", GetLastError());
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

//delete working well ........
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
	WCHAR		full_path[MAX_PATH];
	char		path[MAX_PATH];
	size_t		n = 0;

	GetModuleFileNameW(NULL, full_path, MAX_PATH);
	wcstombs_s(&n, path, MAX_PATH, full_path, wcslen(full_path));
	if (op == 1)
		install_sc(path);
	else if (op == 2)
	{
		if (strchr(path, '\\'))
			memcpy(strrchr(path, '\\') + 1, WINKEY, strlen(WINKEY) + 1);
		else
			memcpy(path, WINKEY, strlen(WINKEY) + 1);
		printf("=======\n%s\n========\n", path);
		start_sc(path);
	}
	else if (op == 4)
	{
		printf("======== delete ======\n");
		delete_sc();
	}
	else
		printf("smtg else\n");
}

int main(int ac, char **av)
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
	return 0;
}