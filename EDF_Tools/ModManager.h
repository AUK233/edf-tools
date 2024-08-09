#pragma once

class CWpnListMgr
{
public:
	void GenerateUI( );
	bool RunUI( );

	static LRESULT CALLBACK WindowFns( HWND window, unsigned int msg, WPARAM wp, LPARAM lp );
	LRESULT CALLBACK HandleWindow( HWND window, unsigned int msg, WPARAM wp, LPARAM lp );

protected:
	void AddButton( const std::wstring& strn );

	void LoadFiles( );
	std::wstring strPathToWeaponList;
	std::wstring strPathToWeaponText;

	//std::map< int, std::function< void( void ) > > fns;

	HINSTANCE hInstance;
	HWND window;
};