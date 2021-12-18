// zoom-plugin.cpp : Defines the entry point for the application.
//
#pragma warning( disable : 26812 )

#include "framework.h"
#include "zoom-plugin.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInstance;                            // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND g_hWndBtnStart;
HWND g_hWndBtnReloadAllowedUserList;
HWND g_hWndBtnSaveUserListToDict;

BOOL                InitInstance(int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


class CustomUIFlow : public ICustomUIFlow
{
	//IAuthServiceEvent
	void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret)
	{
		if (ret == ZOOM_SDK_NAMESPACE::AuthResult::AUTHRET_SUCCESS)
		{
			OutputDebugString(L"SDK Authentificated.");
		}
	}

	void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason)
	{
		if (ret == ZOOM_SDK_NAMESPACE::LOGINSTATUS::LOGIN_SUCCESS)
		{
			EnableWindow(g_hWndBtnStart, TRUE);

			OutputDebugString(L"User authentificated.");
		}
	}
};

CustomUIFlow g_customUIFlow;
std::unique_ptr<SDKInterfacePlugin> g_pSdkInterfacePlugin;



int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	hInstance = hInst;

	SDKUserDatabase::GetInst().SetPath(L"..\\database.json");
	SDKUserDatabase::GetInst().InitializeDatabase();

	ICustomUIFlow* pICustomUIFlow = dynamic_cast<ICustomUIFlow*>(&g_customUIFlow);
	g_pSdkInterfacePlugin = std::make_unique<SDKInterfacePlugin>(pICustomUIFlow);

	if (!g_pSdkInterfacePlugin->InitializeZoomPlugin())
	{
		return 1;
	}

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_ZOOMPLUGIN, szWindowClass, MAX_LOADSTRING);

	// Perform application initialization:
	if (!InitInstance(nCmdShow))
	{
		return 1;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ZOOMPLUGIN));
	MSG msg;

	// Main message loop:
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



// Win32 registered window functions
BOOL InitInstance(int nCmdShow)
{
	WNDCLASSEXW wcex{};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ZOOMPLUGIN));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ZOOMPLUGIN);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindow(
		szWindowClass, // Predefined class; Unicode assumed 
		szTitle,			  // Text
		WS_OVERLAPPEDWINDOW,  // Styles 
		CW_USEDEFAULT,		  // x position 
		0,					  // y position 
		360,				  // width
		256,				  // height
		nullptr,			  // Parent window
		nullptr,			  // No menu.
		hInstance,
		nullptr);

	if (!hWnd) return FALSE;

	g_hWndBtnStart = CreateWindowW(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Join Host Meeting",   // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED | BS_MULTILINE,  // Styles 
		10,         // x position 
		10,         // y position 
		100,        // Button width
		100,        // Button height
		hWnd,       // Parent window
		NULL,       // No menu.
		hInstance,
		NULL);      // Pointer not needed.

	g_hWndBtnReloadAllowedUserList = CreateWindowW(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Reload Allowed User List",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_MULTILINE,  // Styles 
		120,        // x position 
		10,         // y position 
		100,        // Button width
		100,        // Button height
		hWnd,       // Parent window
		NULL,       // No menu.
		hInstance,
		NULL);      // Pointer not needed.

	g_hWndBtnSaveUserListToDict = CreateWindowW(
		L"BUTTON",  // Predefined class; Unicode assumed 
		L"Merge User List To Dict",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_MULTILINE,  // Styles 
		230,        // x position 
		10,         // y position 
		100,        // Button width
		100,        // Button height
		hWnd,       // Parent window
		NULL,       // No menu.
		hInstance,
		NULL);      // Pointer not needed.


	if (!g_hWndBtnStart || 
		!g_hWndBtnReloadAllowedUserList || 
		!g_hWndBtnSaveUserListToDict)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}


void HandleButtonClick(LPARAM lParam)
{
	if (lParam == (long)g_hWndBtnStart)
	{
		g_pSdkInterfacePlugin->JoinMeetingPluginUser();
	}
	else if (lParam == (long)g_hWndBtnReloadAllowedUserList)
	{
		SDKUserDatabase::GetInst().InitializeDatabase();
	}
	else if (lParam == (long)g_hWndBtnSaveUserListToDict)
	{
		SDKUserDatabase::GetInst().MergeUserListOnDisk();
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);

		switch (wmId)
		{
		case BN_CLICKED:
			HandleButtonClick(lParam);
			break;
		case IDM_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		SDKUserDatabase::GetInst().MergeUserListOnDisk();
		PostQuitMessage(0);
		break;
	default: return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}