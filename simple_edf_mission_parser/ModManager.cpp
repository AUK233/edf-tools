#include "stdafx.h"
#include <Windows.h>
#include <string>
#include <iostream>
#include "ModManager.h"

long __stdcall CWpnListMgr::WindowFns( HWND window, unsigned int msg, WPARAM wp, LPARAM lp )
{
	CWpnListMgr* me = (CWpnListMgr*)( GetWindowLongPtr( window, GWLP_USERDATA ) );
	if( me )
		return me->HandleWindow( window, msg, wp, lp );
	else
	{
		if( msg == WM_NCCREATE )
		{
			SetWindowLongPtr( window, GWLP_USERDATA, (LONG)( lp ) );
		}
	}
	return DefWindowProc( window, msg, wp, lp );
}

long __stdcall CWpnListMgr::HandleWindow( HWND window, unsigned int msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	case WM_CREATE:
	{
		HMENU hMenu = CreateMenu( );
		HMENU hSubMenu = CreatePopupMenu( );

		AppendMenu( hSubMenu, MF_STRING, 9001, LPCWSTR("Open") );
		AppendMenu( hSubMenu, MF_STRING, 9002, LPCWSTR( "Exit" ) );
		AppendMenu( hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, LPCWSTR( "File" ) );

		AppendMenu( hMenu, MF_STRING, 9003, LPCWSTR( "Help" ) );

		SetMenu( window, hMenu );
	}
	break;

	case WM_DESTROY:
		//std::wcout << L"\ndestroying window\n";
		PostQuitMessage( 0 );
	return 0L;

	case WM_LBUTTONDOWN:
	//std::wcout << L"\nmouse left button down at (" << LOWORD( lp )
	//	<< L',' << HIWORD( lp ) << L")\n";
	// fall thru

	case WM_COMMAND:
	{
		switch( LOWORD( wp ) )
		{
		case 9001:
			LoadFiles( );
		}
	}
	break;

	default:
	//std::wcout << L'.';
	return DefWindowProc( window, msg, wp, lp );
	}
}

void CWpnListMgr::GenerateUI( )
{
	//Get current instance
	hInstance = GetModuleHandle( NULL );

	//Create window class:
	WNDCLASSEX wc;
	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = 0;
	wc.lpfnWndProc = CWpnListMgr::WindowFns; // WindowFns;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"ModManager";
	wc.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

	if( !RegisterClassEx( &wc ) )
	{
		MessageBox( NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK );
		return;
	}

	window = CreateWindowEx(
		WS_EX_APPWINDOW,
		L"ModManager",
		L"Window",
		WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		640,
		420,
		NULL,
		NULL,
		hInstance,
		(LPVOID)(this)
	);

	AddButton( L"PLSHELPME" );

	ShowWindow( window, SW_SHOW );

	while( RunUI( ) )
	{

	}
}

bool CWpnListMgr::RunUI( )
{
	MSG msg;
	BOOL success = GetMessage( &msg, 0, 0, 0 );
	if( success )
		DispatchMessage( &msg );

	return success;
}

void CWpnListMgr::AddButton( std::wstring strn )
{
	HWND hwndButton = CreateWindowW(
		L"BUTTON",  // Predefined class; Unicode assumed 
		( LPCWSTR )strn.c_str(),      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		10,         // x position 
		10,         // y position 
		100,        // Button width
		100,        // Button height
		window,     // Parent window
		NULL,       // No menu.
		(HINSTANCE)GetWindowLong( window, GWL_HINSTANCE ),
		NULL );      // Pointer not needed.
}

std::wstring OpenFileDialogue( wchar_t *filter = L"All Files (*.*)\0*.*\0", HWND owner = NULL )
{
	OPENFILENAMEW ofn;
	wchar_t fileName[MAX_PATH] = L"";
	ZeroMemory( &ofn, sizeof( ofn ) );
	ofn.lStructSize = sizeof( OPENFILENAME );
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"";
	std::wstring fileNameStr;

	if( GetOpenFileNameW( &ofn ) )
		fileNameStr = fileName;
	return fileNameStr;
}
void CWpnListMgr::LoadFiles( )
{
	std::wstring file = OpenFileDialogue( L"Weapon List (*.json, *.sgo)\0*.json;*.sgo\0" );

	strPathToWeaponList = file;
	//get path:

}
