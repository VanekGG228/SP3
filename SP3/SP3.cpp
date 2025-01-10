#include "framework.h"
#include "SP3.h"
#include <string>

using namespace std;

#define MAX_LOADSTRING 100



HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SP3, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SP3));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SP3));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

inline HKEY GetRootKey(const wstring& rootKeyStr) {
    if (rootKeyStr == L"HKEY_CURRENT_USER") {
        return HKEY_CURRENT_USER;
    }
    else if (rootKeyStr == L"HKEY_LOCAL_MACHINE") {
        return HKEY_LOCAL_MACHINE;
    }
    else if (rootKeyStr == L"HKEY_CLASSES_ROOT") {
        return HKEY_CLASSES_ROOT;
    }
    else if (rootKeyStr == L"HKEY_USERS") {
        return HKEY_USERS;
    }
    else if (rootKeyStr == L"HKEY_CURRENT_CONFIG") {
        return HKEY_CURRENT_CONFIG;
    }
    else {
        return nullptr;
    }
}

inline LONG getKey(const std::wstring rootKeyStr, const wstring& fullPath, HKEY* hKey) {

    HKEY rootKey = GetRootKey(rootKeyStr);
    if (!rootKey) {
        return ERROR_INVALID_PARAMETER;
    }
    return RegOpenKeyEx(rootKey, fullPath.c_str(), 0, KEY_ALL_ACCESS, hKey);
}

wstring getByPath(const wstring& root, const wstring& path, const wstring& keyName, DWORD &type) {
    HKEY hKey;
    if (LONG result = getKey(root,path, &hKey);
        result == ERROR_SUCCESS) {

        wchar_t data[256];
        DWORD dataSize = sizeof(data);
        

        result = RegQueryValueEx(hKey, keyName.c_str(), nullptr, &type, (LPBYTE)data, &dataSize);
        if (type == REG_DWORD || type == REG_QWORD) {

            return std::to_wstring(data[0]);
        }
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS) return data;
        else return L"Неверное имя";
    }
    return L"Ключ не найден";
}

BOOL createRegistryKey(const wstring& root, const wstring& path, const wstring& paramName, const wstring& paramValue, DWORD type) {
    HKEY hKey;
 
    DWORD flag = 0;
    auto rootKey = GetRootKey(root);
    LONG result = RegCreateKeyEx(
        rootKey,
        path.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &flag
    );
    if (flag == REG_CREATED_NEW_KEY || flag == REG_OPENED_EXISTING_KEY) {
        if (type == REG_DWORD || type == REG_QWORD) {
            ULONGLONG dwValue = std::stoul(paramValue);
            RegSetValueEx(
                hKey,
                paramName.c_str(),
                0,
                type,
                reinterpret_cast<const BYTE*>(&dwValue),
                sizeof(DWORD)
            );
        }
        else {
            RegSetValueEx(
                hKey,
                paramName.c_str(),
                0,
                type,
                reinterpret_cast<const BYTE*>(paramValue.c_str()),
                static_cast<DWORD>((paramValue.length() + 1) * sizeof(wchar_t))
            );
        }
    }
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

BOOL deleteByPath(const wstring& root,const wstring& path, const wstring& keyName) {
    HKEY hKey;
    if (LONG result = getKey(root,path, &hKey);
        result == ERROR_SUCCESS) {

        result = RegDeleteValue(hKey, keyName.c_str());
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS) {
            return TRUE;
        }
    }

    return FALSE;
}

DWORD getType(const HWND hEdit) {
    wchar_t buffer[256];

    GetWindowText(hEdit, buffer, sizeof(buffer) / sizeof(buffer[0]));
    std::wstring typeStr{ buffer };

    if (typeStr == L"REG_NONE") {
        return REG_NONE;
    }
    else if (typeStr == L"REG_SZ") {
        return REG_SZ;
    }
    else if (typeStr == L"REG_EXPAND_SZ") {
        return REG_EXPAND_SZ;
    }
    else if (typeStr == L"REG_BINARY") {
        return REG_BINARY;
    }
    else if (typeStr == L"REG_DWORD" || typeStr == L"REG_DWORD_LITTLE_ENDIAN") {
        return REG_DWORD;
    }
    else if (typeStr == L"REG_DWORD_BIG_ENDIAN") {
        return REG_DWORD_BIG_ENDIAN;
    }
    else if (typeStr == L"REG_LINK") {
        return REG_LINK;
    }
    else if (typeStr == L"REG_MULTI_SZ") {
        return REG_MULTI_SZ;
    }
    else if (typeStr == L"REG_RESOURCE_LIST") {
        return REG_RESOURCE_LIST;
    }
    else if (typeStr == L"REG_FULL_RESOURCE_DESCRIPTOR") {
        return REG_FULL_RESOURCE_DESCRIPTOR;
    }
    else if (typeStr == L"REG_RESOURCE_REQUIREMENTS_LIST") {
        return REG_RESOURCE_REQUIREMENTS_LIST;
    }
    else if (typeStr == L"REG_QWORD" || typeStr == L"REG_QWORD_LITTLE_ENDIAN") {
        return REG_QWORD;
    }

    return REG_SZ;
}

std::wstring getTypeStr(const DWORD type) {
    switch (type) {
    case REG_NONE:
        return L"REG_NONE";
    case REG_SZ:
        return L"REG_SZ";
    case REG_EXPAND_SZ:
        return L"REG_EXPAND_SZ";
    case REG_BINARY:
        return L"REG_BINARY";
    case REG_DWORD:
        return L"REG_DWORD";
    case REG_DWORD_BIG_ENDIAN:
        return L"REG_DWORD_BIG_ENDIAN";
    case REG_LINK:
        return L"REG_LINK";
    case REG_MULTI_SZ:
        return L"REG_MULTI_SZ";
    case REG_RESOURCE_LIST:
        return L"REG_RESOURCE_LIST";
    case REG_FULL_RESOURCE_DESCRIPTOR:
        return L"REG_FULL_RESOURCE_DESCRIPTOR";
    case REG_RESOURCE_REQUIREMENTS_LIST:
        return L"REG_RESOURCE_REQUIREMENTS_LIST";
    case REG_QWORD:
        return L"REG_QWORD";
    default:
        return L"UNKNOWN_TYPE"; // Для неизвестных типов можно вернуть "UNKNOWN_TYPE"
    }
}


std::wstring getdata(const HWND hEdit) {
    wchar_t buffer[256];
    GetWindowText(hEdit, buffer, sizeof(buffer) / sizeof(buffer[0]));
    std::wstring path{ buffer }; 
    return path;
}

std::wstring GetSelectedRootKey(const HWND &hComboBox) {
    wchar_t buffer[256];
    GetWindowText(hComboBox, buffer, sizeof(buffer) / sizeof(buffer[0]));
    std::wstring path{ buffer };
    return path;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hLabel, hEdit, hButton;
    static HWND hLabel2, hEdit2, hButton2;
    static HWND hLabel3, hEdit3, hButton3;
    static HWND hLabel4, hEdit4, hCombo; 
    static HFONT hFont;

   
    switch (message)
    {
    case WM_CREATE:
    {

        hCombo = CreateWindowW(L"COMBOBOX", nullptr,
            WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWN | CBS_HASSTRINGS,
            10, 20, 250, 150,  
            hWnd, (HMENU)ID_COMBO, NULL, NULL);


        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"HKEY_CURRENT_USER");
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"HKEY_LOCAL_MACHINE");
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"HKEY_CLASSES_ROOT");
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"HKEY_USERS");
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"HKEY_CURRENT_CONFIG");

        hLabel = CreateWindowW(L"STATIC", L"Введите путь ключа реестра:",
            WS_VISIBLE | WS_CHILD,
            10, 70, 250, 30,
            hWnd, (HMENU)ID_LABEL1, NULL, NULL);

        hEdit = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            260, 70, 700, 30,
            hWnd, (HMENU)ID_EDIT1, NULL, NULL);

        hLabel2 = CreateWindowW(L"STATIC", L"Введите имя ключа:",
            WS_VISIBLE | WS_CHILD,
            10, 120, 250, 30,
            hWnd, (HMENU)ID_LABEL2, NULL, NULL);

        hEdit2 = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            260, 120, 700, 30,
            hWnd, (HMENU)ID_EDIT2, NULL, NULL);

        hLabel3 = CreateWindowW(L"STATIC", L"Значение ключа:",
            WS_VISIBLE | WS_CHILD,
            10, 170, 250, 30,
            hWnd, (HMENU)ID_LABEL3, NULL, NULL);

        hEdit3 = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            260, 170, 700, 30,
            hWnd, (HMENU)ID_EDIT3, NULL, NULL);

        hLabel4 = CreateWindowW(L"STATIC", L"Тип значения ключа:",
            WS_VISIBLE | WS_CHILD,
            10, 220, 250, 30,
            hWnd, (HMENU)ID_LABEL4, NULL, NULL);

        hEdit4 = CreateWindowW(L"EDIT", L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
            260, 220, 700, 30,
            hWnd, (HMENU)ID_EDIT4, NULL, NULL);

        hButton = CreateWindowW(L"BUTTON", L"Найти ключ",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            100, 300, 160, 50,
            hWnd, (HMENU)ID_SEARCH_BUTTON, NULL, NULL);

        hButton2 = CreateWindowW(L"BUTTON", L"Установить ключ",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 300, 160, 50,
            hWnd, (HMENU)ID_CREATE_BUTTON, NULL, NULL);

        hButton3 = CreateWindowW(L"BUTTON", L"Удалить ключ",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            500, 300, 160, 50,
            hWnd, (HMENU)ID_DELETE_BUTTON, NULL, NULL);

        hFont = CreateFont(
            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");

        SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabel2, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabel3, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabel4, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit3, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEdit4, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hButton2, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hButton3, WM_SETFONT, (WPARAM)hFont, TRUE);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_SEARCH_BUTTON:
        {
            DWORD type;
            std::wstring res = getByPath(GetSelectedRootKey(hCombo), getdata(hEdit), getdata(hEdit2), type);
            SetWindowText(hEdit3, res.c_str());
            SetWindowText(hEdit4, getTypeStr(type).c_str());
            break;
        }
        case ID_CREATE_BUTTON:
        {
            DWORD type = getType(hEdit4);
            if (auto result = createRegistryKey(GetSelectedRootKey(hCombo),getdata(hEdit), getdata(hEdit2), getdata(hEdit3), type);
                result == TRUE) {
                MessageBox(nullptr, L"Установлено успешно", L"Успех",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBox(nullptr, L"Ошибка установки", L"Ошибка",
                    MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case ID_DELETE_BUTTON:
        {
            if (auto result = deleteByPath(GetSelectedRootKey(hCombo), getdata(hEdit), getdata(hEdit2));
                result == TRUE) {
                MessageBox(nullptr, L"Удалено успешно", L"Успех",
                    MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBox(nullptr, L"Не удалось удалить", L"Ошибка",
                    MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;

        mmi->ptMinTrackSize.x = 1000;
        mmi->ptMinTrackSize.y = 600;

        mmi->ptMaxTrackSize.x = 1000;
        mmi->ptMaxTrackSize.y = 600;
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}