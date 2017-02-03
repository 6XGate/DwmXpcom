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
#include <windows.h>
#include "VirtualMemory.h"

#pragma pack(push, 1)
struct EventThunk
{
#if defined(_M_IX86)
	DWORD Mov;
	DWORD This;
	BYTE Jmp;
	DWORD RelProc;

	static DWORD MovOp;
	static BYTE JmpOp;
#elif defined(_M_AMD64)
	USHORT MovRcx;
	UINT_PTR This;
	USHORT MovRax;
	UINT_PTR Proc;
	USHORT JmpRax;

	static USHORT MovRcxVal;
	static USHORT MovRaxVal;
	static USHORT JmpRaxVal;
#else
#	error Only AMD64 and X86 supported
#endif
};
#pragma pack(pop)

extern VirtualMemory<EventThunk, PAGE_EXECUTE_READWRITE> ThunkAllocator;

typedef HRESULT (STDAPICALLTYPE*DwmIsCompositionEnabledSig)(__out BOOL * pfEnabled);
