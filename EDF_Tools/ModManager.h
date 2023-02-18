#pragma once

class CWpnListMgr
{
public:
	void GenerateUI( );
	bool RunUI( );

	static long __stdcall WindowFns( HWND window, unsigned int msg, WPARAM wp, LPARAM lp );
	long __stdcall HandleWindow( HWND window, unsigned int msg, WPARAM wp, LPARAM lp );

protected:
	void AddButton( std::wstring strn );

	void LoadFiles( );
	std::wstring strPathToWeaponList;
	std::wstring strPathToWeaponText;

	//std::map< int, std::function< void( void ) > > fns;

	HINSTANCE hInstance;
	HWND window;
};