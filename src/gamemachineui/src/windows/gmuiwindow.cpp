﻿#include "stdafx.h"
#include "gmui.h"
#include <gamemachine.h>
#include "gmuiwindow.h"
#include "gmuiinput.h"

GMUIWindow::~GMUIWindow()
{
	D(d);
	if (d->input)
		delete d->input;
}

gm::IInput* GMUIWindow::getInputMananger()
{
	D(d);
	if (!d->input)
		d->input = new GMInput(this);
	return d->input;
}

bool GMUIWindow::handleMessage()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	return true;
}

void GMUIWindow::update()
{
	D(d);
	if (d->input)
		d->input->update();
}

gm::GMRect GMUIWindow::getWindowRect()
{
	RECT rect;
	GetWindowRect(getWindowHandle(), &rect);
	gm::GMRect r = { rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
	return r;
}

gm::GMRect GMUIWindow::getClientRect()
{
	RECT rect;
	GetClientRect(getWindowHandle(), &rect);
	gm::GMRect r = { rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
	return r;
}

void GMUIWindow::centerWindow() 
{
	gm::GMWindowHandle hWnd = getWindowHandle();
	GM_ASSERT(::IsWindow(hWnd));
	GM_ASSERT((GetWindowStyle(hWnd)&WS_CHILD) == 0);
	RECT rcDlg = { 0 };
	::GetWindowRect(hWnd, &rcDlg);
	RECT rcArea = { 0 };
	RECT rcCenter = { 0 };
	HWND hWndParent = ::GetParent(hWnd);
	HWND hWndCenter = ::GetWindowOwner(hWnd);
	if (hWndCenter != NULL)
		hWnd = hWndCenter;

	// 处理多显示器模式下屏幕居中
	MONITORINFO oMonitor = {};
	oMonitor.cbSize = sizeof(oMonitor);
	::GetMonitorInfo(::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &oMonitor);
	rcArea = oMonitor.rcWork;

	if (hWndCenter == NULL || IsIconic(hWndCenter))
		rcCenter = rcArea;
	else
		::GetWindowRect(hWndCenter, &rcCenter);

	int DlgWidth = rcDlg.right - rcDlg.left;
	int DlgHeight = rcDlg.bottom - rcDlg.top;

	// Find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// The dialog is outside the screen, move it inside
	if (xLeft < rcArea.left) xLeft = rcArea.left;
	else if (xLeft + DlgWidth > rcArea.right) xLeft = rcArea.right - DlgWidth;
	if (yTop < rcArea.top) yTop = rcArea.top;
	else if (yTop + DlgHeight > rcArea.bottom) yTop = rcArea.bottom - DlgHeight;
	::SetWindowPos(hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool GMUIWindow::isWindowActivate()
{
	return ::GetForegroundWindow() == getWindowHandle();
}

void GMUIWindow::setLockWindow(bool lock)
{
	if (lock)
		::SetCapture(getWindowHandle());
	else
		::ReleaseCapture();
}

LRESULT CALLBACK GMUIWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	GMUIWindow* pGMWindow = nullptr;
	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pGMWindow = static_cast<GMUIWindow*>(lpcs->lpCreateParams);
		pGMWindow->setWindowHandle(hWnd);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pGMWindow));
	}
	else
	{
		pGMWindow = reinterpret_cast<GMUIWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	bool handled;
	if (pGMWindow)
	{
		LRESULT result = pGMWindow->wndProc(uMsg, wParam, lParam, handled);
		if (handled)
			return result;
	}
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}