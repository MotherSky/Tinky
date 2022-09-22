#include <string.h>
# include <windows.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <tlhelp32.h>
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("tinky")
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

int assign_scmanager()
{
	int err;

	scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!scManager)
	{
		err = GetLastError();
		if (err == ERROR_ACCESS_DENIED)
			printf("No Admin rights \n");
		else
			printf("OpenSCManager failed error (%ld)\n", GetLastError());
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
		start_sc(path);
	else
		printf("smtg else\n");
}

int main(int ac, char **av)
{
	long int	err;
	int			op;

	if ((op = usage(ac, av)))
	{
		if (assign_scmanager())
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