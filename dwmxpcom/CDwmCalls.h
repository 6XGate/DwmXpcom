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
#pragma once
#include "IDwmCalls.h"
#include "nsIBaseWindow.h"
#include "dwmxpcom.h"

#define CDWMCALLS_CONTRACTID "@sixxgate.com/DwmCalls/CDwmCalls;1"
#define CDWMCALLS_CLASSNAME "CDwmCalls"
// {F1D9A539-FD93-4062-AAAA-FB53CB66C054}
#define CDWMCALLS_CID { 0xF1D9A539, 0xFD93, 0x4062, { 0xAA, 0xAA, 0xFB, 0x53, 0xCB, 0x66, 0xC0, 0x54 } }
 
class CDwmCalls : public IDwmCalls
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IDWMCALLS

  CDwmCalls();

private:
  ~CDwmCalls();

protected:
	HMODULE _dwmApiLib;
	DwmIsCompositionEnabledSig _dwmIsCompositionEnabled;
	EventThunk * _thunk;
	nsIDOMWindow * _win;
	WNDPROC _oldProc;
	HWND _hWnd;
	nsIDOMEventListener * _onTheme;

	NS_IMETHODIMP _GetBaseWin(nsIDOMWindow * win, nsIBaseWindow ** baseWin);
	static LRESULT CALLBACK ThemeEventProc(CDwmCalls *, UINT msg, WPARAM, LPARAM);
	WNDPROC _CreateEventProc();
};
