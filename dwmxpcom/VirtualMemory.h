/******************************************************************************
Virtual Memory Manager Template
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
#include <cassert>
#include <cstring>
#include <tchar.h>
#include <windows.h>

template <class> struct _VmItem;

// Virtual Memory block header
template <class _Type>
struct _VmBlock
{
	typedef _VmItem<_Type> _ElemTyp;
	typedef _VmBlock<_Type> _MyTyp;

	// Free element stack, tracks availible elements
	size_t * StackBeg;
	size_t * StackPtr;
	size_t * StackLast;

	// First and last element in the block
	_ElemTyp * FirstElem;

	// Next or previous block in the list
	_MyTyp * NextBlock;
	_MyTyp * PrevBlock;
};

#define NoMansVal 0xFDFDFDFD

// Element, the object type with a footer to reference it's owning block
template <class _Type>
struct _VmItem
{
	_Type x;
#if defined(_DEBUG)
	DWORD _noMans;
#endif//_DEBUG
};

template <class _Type, DWORD _prot = PAGE_READWRITE>
class VirtualMemory
{
public:
	typedef typename _VmBlock<_Type>::_ElemTyp _ElemTyp;
	typedef typename _VmBlock<_Type>::_MyTyp _BlockTyp;
	typedef VirtualMemory<_Type> _MyTyp;

	VirtualMemory() : _listHead(NULL), _listTail(NULL)
	{	// Gets the system information needed to manage virtual memory for _Type
		SYSTEM_INFO info;
		::GetSystemInfo(&info);
		_sysGran = info.dwAllocationGranularity;
	}

	~VirtualMemory()
	{
		if (_listHead == NULL)
			return;

		::OutputDebugString(_T("Memory objects are still allocated"));
		_BlockTyp * nextBlock = _listHead->NextBlock;
		for (; nextBlock != NULL; nextBlock = _listHead->NextBlock)
		{
			::VirtualFree(_listHead, _sysGran, MEM_DECOMMIT | MEM_RELEASE);
			_listHead = nextBlock;
		}
	}

	_Type * New()
	{
		// Find a block with a free element
		_VmBlock<_Type> * block = _FindBlock();

		// If no block was found or none are allocated, create a new one
		if (block == NULL)
			block = _NewBlock();

		size_t index = *(block->StackPtr--);
		_ElemTyp * elem = &block->FirstElem[index];

#if defined(_DEBUG)
		std::memset(elem,  0xCD, sizeof(_Type));
		elem->_noMans = NoMansVal;
#endif//_DEBUG

		return (reinterpret_cast<_Type*>(elem));
	}

	_Type * New(const _Type & rhs)
	{
		_Type * ptr = New();
		::new (ptr) _Type(rhs);
		return (ptr);
	}

	void Delete(_Type * ptr)
	{
		if (ptr == NULL)
			return;

		_ElemTyp * elem = reinterpret_cast<_ElemTyp*>(ptr);
		_BlockTyp * block = (_BlockTyp*)((uintptr_t)elem / (uintptr_t)_sysGran
			* (uintptr_t)_sysGran);

		assert(elem->_noMans == NoMansVal);

		*(++block->StackPtr) = elem - block->FirstElem;

		if (block->StackPtr == block->StackLast)
		{
			if (block->NextBlock != NULL)
				block->NextBlock->PrevBlock = block->PrevBlock;
			else
				_listTail = block->PrevBlock;

			if (block->PrevBlock != NULL)
				block->PrevBlock->NextBlock = block->NextBlock;
			else
				_listHead = block->NextBlock;

			::VirtualFree(block, _sysGran, MEM_DECOMMIT | MEM_RELEASE);
		}

	}

private:
	_BlockTyp * _listHead;
	_BlockTyp * _listTail;
	DWORD _sysGran;

	_BlockTyp * _FindBlock()
	{
		_BlockTyp * block = _listHead;
		for (; block != NULL; block = block->NextBlock)
			if (block->StackPtr >= block->StackBeg)
				return block;

		return block;
	}

	_VmBlock<_Type> * _NewBlock()
	{
		_BlockTyp * newBlock = (_BlockTyp*)::VirtualAlloc(NULL, _sysGran,
			MEM_COMMIT | MEM_RESERVE, _prot);

		size_t count = (_sysGran - sizeof(_BlockTyp))
			/ (sizeof(_ElemTyp) + sizeof(size_t));
		
		newBlock->StackBeg = (size_t*)(newBlock + 1);
		newBlock->StackPtr = newBlock->StackBeg;
		newBlock->StackLast = newBlock->StackBeg + (count - 1);

		for (size_t i = 0; i != count; ++i)
			*(newBlock->StackPtr++) = i;
		--newBlock->StackPtr;

		newBlock->FirstElem = (_ElemTyp*)(newBlock->StackLast + 1);

		if (_listHead == NULL)
		{
			_listHead = _listTail = newBlock;

			newBlock->NextBlock = NULL;
			newBlock->PrevBlock = NULL;
		}
		else
		{
			newBlock->NextBlock = NULL;
			newBlock->PrevBlock = _listTail;
			_listTail->NextBlock = newBlock;
			_listTail = _listTail->NextBlock;
		}

		return newBlock;
	}
};