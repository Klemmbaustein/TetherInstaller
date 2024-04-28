// AppInstaller.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "prsht.h"
#include "AppInstaller.h"
#include "winuser.h"
#include "Commctrl.h"
#include "Install.h"
#include "shellapi.h"

#define MAX_LOADSTRING 100
#define PROGRESSBAR_TIMER_ID 1

// Global Variables:
HINSTANCE hInst;                                // current instance

VOID DoPropertySheet();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					 _In_opt_ HINSTANCE hPrevInstance,
					 _In_ LPWSTR    lpCmdLine,
					 _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DoPropertySheet();

	return (int)0;
}


static int CurrentPage = 0;
static bool OnDesktop = false, OnStartMenu = false;

INT_PTR CALLBACK PageCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TIMER:
	{
		/*
		 * Catch the timer event that is fired each second. Increment the progress
		 * bar by 10% each time.
		 */
		HWND hwndProgressBar = GetDlgItem(hWnd, IDC_PROGRESS1);
		UINT iPos = SendMessage(hwndProgressBar, PBM_GETPOS, 0, 0);

		/*
		 * If the position is already full then kill the timer. Else increment the
		 * progress bar.
		 */
		if (!Install::IsInstalling())
		{
			KillTimer(hwndProgressBar, PROGRESSBAR_TIMER_ID);
			SendMessage(hwndProgressBar, PBM_SETPOS, 100, 0);
			PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_BACK | PSWIZB_FINISH);
		}
		else
		{
			SendMessage(hwndProgressBar, PBM_SETPOS, iPos + 10, 0);
		}

		break;
	}
	case WM_NOTIFY:
	{
		SendMessage(hWnd, PBM_STEPIT, 0, 0);
		LPNMHDR pnmh = (LPNMHDR)lParam;
		
		switch (pnmh->code)
		{
		case PSN_SETACTIVE:
			// This is an interior page.
			if (CurrentPage == 0)
			{
				PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_NEXT);
			}
			else if (CurrentPage != 2)
			{
				PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_NEXT | PSWIZB_BACK);
				CheckDlgButton(hWnd, IDC_DESKTOP, TRUE);
				CheckDlgButton(hWnd, IDC_STARTMENU, TRUE);
			}
			else
			{
				PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_BACK | PSWIZB_DISABLEDFINISH);
				SetTimer(hWnd, PROGRESSBAR_TIMER_ID, 50, NULL);
				
				Install::InstallApp(OnStartMenu, OnDesktop);
			}
			break;
		case PSN_WIZNEXT:
			switch (CurrentPage)
			{
			case 0:
				SetWindowLong(hWnd, 0, IDD_WIZARD_SETUP_PAGE);
				break;
			case 1:
				OnStartMenu = IsDlgButtonChecked(hWnd, IDC_STARTMENU);
				OnDesktop = IsDlgButtonChecked(hWnd, IDC_DESKTOP);
				SetWindowLong(hWnd, 0, IDD_WIZARD_INSTALLPAGE);
				break;
			default:
				break;
			}
			CurrentPage++;
			break;
		case PSN_WIZBACK:
			CurrentPage--;
			break;
		case PSN_WIZFINISH:
			Install::LaunchApp();
			break;
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

VOID DoPropertySheet()
{
	HPROPSHEETPAGE ahpsp[3];
	PROPSHEETPAGE psp[3] = {sizeof(psp)};

	psp[0].dwSize = sizeof(psp[0]);
	psp[0].hInstance = hInst;
	psp[0].dwFlags = PSP_HIDEHEADER;
	psp[0].lParam = (LPARAM)0;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_WIZARD_EXTERIOR);
	psp[0].pfnDlgProc = PageCallback;

	psp[1].dwSize = sizeof(psp[1]);
	psp[1].hInstance = hInst;
	psp[1].dwFlags = PSP_HIDEHEADER;
	psp[1].lParam = (LPARAM)0;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_WIZARD_SETUP_PAGE);
	psp[1].pfnDlgProc = PageCallback;

	psp[2].dwSize = sizeof(psp[1]);
	psp[2].hInstance = hInst;
	psp[2].dwFlags = PSP_HIDEHEADER;
	psp[2].lParam = (LPARAM)0;
	psp[2].pszTemplate = MAKEINTRESOURCE(IDD_WIZARD_INSTALLPAGE);
	psp[2].pfnDlgProc = PageCallback;

	ahpsp[0] = CreatePropertySheetPage(&psp[0]);
	ahpsp[1] = CreatePropertySheetPage(&psp[1]);
	ahpsp[2] = CreatePropertySheetPage(&psp[2]);

	PROPSHEETHEADER psh = { sizeof(psh) };

	psh.hInstance = hInst;
	psh.hwndParent = NULL;
	psh.phpage = ahpsp;
	psh.dwFlags = PSH_WIZARD97;
	psh.nStartPage = 0;
	psh.nPages = 3;

	PropertySheet(&psh);

	return;
}
