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

int main(int ac, char **av)
{
	long int err;
	if (usage(ac, av))
	{
		if (assign_scmanager())
			return (1);
		if (!CloseServiceHandle(scManager))
		{
			printf("CloseServiceHandle failed error (%ld)\n", GetLastError());
			return (1);
		}
	}
	return 0;
}