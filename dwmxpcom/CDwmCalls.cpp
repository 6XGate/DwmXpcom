/******************************************************************************
Copyright 2008 Matthew Holder. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/
#include "stdafx.h"
#include "CDwmCalls.h"
#include <ciso646>
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsIXULWindow.h"

#if defined(_M_IX86)
DWORD EventThunk::MovOp = 0x042444C7;
BYTE EventThunk::JmpOp = 0xE9;
#elif defined(_M_AMD64)
USHORT EventThunk::MovRcxVal = 0xB948;
USHORT EventThunk::MovRaxVal = 0xB848;
USHORT EventThunk::JmpRaxVal = 0xE0FF;
#else
#error Only AMD64 and X86 supported
#endif


NS_IMPL_ISUPPORTS1(CDwmCalls, IDwmCalls)

CDwmCalls::CDwmCalls() : _dwmIsCompositionEnabled(NULL), _dwmApiLib(NULL),
	_win(NULL), _hWnd(NULL), _oldProc(NULL), _onTheme(NULL), _thunk(NULL)
{
	// At least get the compositor is enabled call as it can't be delay loaded
	HMODULE _dwmApiLib = ::LoadLibraryW(L"dwmapi.dll");
	if (_dwmApiLib != NULL)
	{
		_dwmIsCompositionEnabled = (DwmIsCompositionEnabledSig)
			::GetProcAddress(_dwmApiLib, "DwmIsCompositionEnabled");
	}
}

CDwmCalls::~CDwmCalls()
{
	if (_dwmApiLib != NULL)
		::FreeLibrary(_dwmApiLib);
	if (_win != NULL)
		_win->Release();
	if (_onTheme != NULL)
		_onTheme->Release();
	if (_thunk != NULL)
		ThunkAllocator.Delete(_thunk);
}

/* attribute nsIDOMEventListener OnThemeChange; */
NS_IMETHODIMP CDwmCalls::GetOnGlassChange(nsIDOMEventListener * * aOnThemeChange)
{
	if (aOnThemeChange == NULL)
		return NS_ERROR_INVALID_POINTER;

	_onTheme->AddRef();
	*aOnThemeChange = _onTheme;

    return NS_OK;
}

NS_IMETHODIMP CDwmCalls::SetOnGlassChange(nsIDOMEventListener * aOnThemeChange)
{
	_onTheme = aOnThemeChange;

	if (aOnThemeChange == NULL)
		return NS_OK;

	aOnThemeChange->AddRef();

    return NS_OK;
}

/* readonly attribute boolean GlassEnabled; */
NS_IMETHODIMP CDwmCalls::GetGlassEnabled(PRBool *aGlassEnabled)
{
	if (aGlassEnabled == NULL)
		return NS_ERROR_INVALID_POINTER;
	else if (_dwmIsCompositionEnabled == NULL)
	    return NS_ERROR_NOT_IMPLEMENTED;

	HRESULT res = _dwmIsCompositionEnabled((BOOL*)aGlassEnabled);
	if (FAILED(res))
		return NS_ERROR_FAILURE;
	else
		return NS_OK;
}

/* void AttachToWindow (in nsIDOMWindow win); */
NS_IMETHODIMP CDwmCalls::AttachToWindow(nsIDOMWindow * winPtr)
{
	if (winPtr == NULL)
		return NS_ERROR_INVALID_POINTER;

	// We now reference the window, also lets put it in t smart pointer
	winPtr->AddRef();
	nsCOMPtr<nsIDOMWindow> win = winPtr;

	// Get the base Window
	nsCOMPtr<nsIBaseWindow> baseWin;
	nsresult res = _GetBaseWin(winPtr, getter_AddRefs(baseWin));
	if (NS_FAILED(res))
		return res;
	
	// Get the Window handle;
    HWND hWnd = NULL;
	res = baseWin->GetParentNativeWindow((nativeWindow*)&hWnd);
	if (NS_FAILED(res))
		return res;

	// Store the window we are attaching to
	_hWnd = hWnd;
	_win = winPtr;
	_win->AddRef();

	// Now sub-class the window so we can get theme change messages
	_oldProc = (WNDPROC)::GetWindowLongPtr(_hWnd, GWL_WNDPROC);
	::SetWindowLong(_hWnd, GWL_WNDPROC, (LONG_PTR)_CreateEventProc());
	
	return NS_OK;
}

/* boolean ExtendFrameIntoWindow (in nsIDOMWindow win, in long top, in long right, in long bottom, in long left); */
NS_IMETHODIMP CDwmCalls::ExtendFrameIntoWindow(PRInt32 top, PRInt32 right, PRInt32 bottom, PRInt32 left)
{
	if (_hWnd == NULL or _win == NULL)
		return NS_ERROR_INVALID_POINTER;
	else if (_dwmIsCompositionEnabled == NULL)
		return NS_ERROR_NOT_IMPLEMENTED;

	// Make sure the compositor is enabled
	BOOL enabled = FALSE;
	HRESULT ret = _dwmIsCompositionEnabled(&enabled);
	if (FAILED(ret))
		return NS_ERROR_FAILURE;

	if (enabled == FALSE)
		return NS_OK;

	MARGINS margins;
	margins.cyTopHeight = top;
	margins.cxRightWidth = right;
	margins.cyBottomHeight = bottom;
	margins.cxLeftWidth = left;

	ret = ::DwmExtendFrameIntoClientArea(_hWnd, &margins);
	if (FAILED(ret))
		return NS_ERROR_FAILURE;
	else
		return NS_OK;
}

NS_IMETHODIMP CDwmCalls::_GetBaseWin(nsIDOMWindow * win, nsIBaseWindow ** baseWin)
{
	// Safety first
	if (win == NULL or baseWin == NULL)
		return NS_ERROR_INVALID_POINTER;

	nsresult res;

	nsCOMPtr<nsIInterfaceRequestor> requestor
		= do_QueryInterface(win, &res);
	if (NS_FAILED(res))
		return res;

	nsCOMPtr<nsIWebNavigation> nav;

	nsIID webnavIid = NS_IWEBNAVIGATION_IID;
	res = requestor->GetInterface(webnavIid, getter_AddRefs(nav));
	if (NS_FAILED(res))
		return res;

	nsCOMPtr<nsIDocShellTreeItem> item
		= do_QueryInterface(nav, &res);
	if (NS_FAILED(res))
		return res;

	nsCOMPtr<nsIDocShellTreeOwner> owner;
	res = item->GetTreeOwner(getter_AddRefs(owner));
	if (NS_FAILED(res))
		return res;

	requestor = do_QueryInterface(owner, &res);
	if (NS_FAILED(res))
		return res;

	nsCOMPtr<nsIXULWindow> xulWin;
	nsIID xulWinIid = NS_IXULWINDOW_IID;
	res = requestor->GetInterface(xulWinIid, getter_AddRefs(xulWin));
	if (NS_FAILED(res))
		return res;

	nsCOMPtr<nsIDocShell> docShell;
	res = xulWin->GetDocShell(getter_AddRefs(docShell));
	if (NS_FAILED(res))
		return res;

	nsIID baseWinIid = NS_IBASEWINDOW_IID;
	res = docShell->QueryInterface(baseWinIid, (void**)baseWin);
	if (NS_FAILED(res))
		return res;

	return res;
}

LRESULT CALLBACK CDwmCalls::ThemeEventProc(CDwmCalls * self, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DWMCOMPOSITIONCHANGED and self->_onTheme != NULL)
	{
		self->_onTheme->HandleEvent(NULL);
		return FALSE;
	}

	return ::CallWindowProc(self->_oldProc, self->_hWnd, msg, wParam, lParam);
}

WNDPROC CDwmCalls::_CreateEventProc()
{
	_thunk = ThunkAllocator.New();

#if defined(_M_IX86)
	_thunk->Mov = EventThunk::MovOp;
	_thunk->This = (DWORD)this;
	_thunk->Jmp = EventThunk::JmpOp;
	_thunk->RelProc = (intptr_t)&CDwmCalls::ThemeEventProc
		- ((intptr_t)_thunk + sizeof(EventThunk));
#elif defined(_M_AMD64)
	_thunk->MovRcx = EventThunk::MovRcxVal;
	_thunk->This = (UINT_PTR)this;
	_thunk->MovRax = EventThunk::MovRaxVal;
	_thunk->Proc = (UINT_PTR)&CDwmCalls::ThemeEventProc;
	_thunk->JmpRax = EventThunk::JmpRaxVal;
#else
#	error Only AMD64 and X86 supported
#endif

	::FlushInstructionCache(::GetCurrentProcess(), _thunk, sizeof(EventThunk));

	return (WNDPROC)_thunk;
}
