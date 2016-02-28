//////////////////////////////////////////////////////////////////////
// tstring.hpp: interface for the tstring class and helper namespace
//////////////////////////////////////////////////////////////////////
// Created by Ilan Gavriel 
//////////////////////////////////////////////////////////////////////
// Description:
// TCHAR support for STL String and Streams
// * replace_all - replace a given string with another
// * upper - convert the given tstring object to upper case
// * lower - convert the given tstring object to lower case
// * trim_right - trim the right hand of whitespace characters
// * trim_left - trim the left hand of whitespace characters
// * trim_all - trim leading and trailing whitespace characters
// * to_char - convert tstring to std::string
// * to_wchar - convert tstring to std::wstring
//
//////////////////////////////////////////////////////////////////////
// History:
// 05/04/2009 - first version
//////////////////////////////////////////////////////////////////////
#ifndef __TSTRING_HPP__
#define __TSTRING_HPP__

#include <TCHAR.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

typedef std::basic_string<TCHAR>	tstring;
typedef std::basic_iostream<TCHAR>	tiostream;
typedef std::basic_istream<TCHAR>	tistream;
typedef std::basic_ifstream<TCHAR>	tifstream;
typedef std::basic_ostream<TCHAR>	tostream;
typedef std::basic_ofstream<TCHAR>	tofstream;

#ifdef _UNICODE
	#define tcin	std::wcin
	#define tcout	std::wcout
	#define tcerr	std::wcerr
#else
	#define tcin	std::cin
	#define tcout	std::cout
	#define tcerr	std::cerr
#endif

//========================================================================
// tstring_helper - namespace with helper functions
//========================================================================
namespace tstring_helper
{
	// replace a given string with another
	inline void replace_all(tstring& input_string, const TCHAR* szFrom, const TCHAR* szTo)
	{
		const size_t nLengthTo =	_tcslen(szTo);
		const size_t nLengthFrom =	_tcslen(szFrom);
		size_t nPos = input_string.find_first_of(szFrom);
		while(nPos != tstring::npos)
		{
			input_string.erase(nPos, nLengthFrom);
			input_string.insert(nPos, szTo);
			nPos = input_string.find_first_of(szFrom, nPos+nLengthTo );
		}
	}

	// convert the given tstring object to upper case
	inline tstring& upper(tstring& input_string)
	{
		std::transform(input_string.begin(), input_string.end(), input_string.begin(), toupper);
		return input_string;
	}

	// convert the given tstring object to lower case
	inline tstring& lower(tstring& input_string)
	{
		std::transform(input_string.begin(), input_string.end(), input_string.begin(), tolower);
		return input_string;
	}

	// trim the right hand of whitespace characters for the given tstring
	inline tstring& trim_right(tstring& input_string, const TCHAR* szDelimiters = _T(" \t\r\n") )
	{
		tstring::size_type nPos = input_string.find_last_not_of(szDelimiters);
		if (tstring::npos != nPos)
		{
			input_string.erase(++nPos);
		}
		return input_string;
	}

    // trim the left hand of whitespace characters for the given tstring
    inline tstring& trim_left(tstring& input_string, const TCHAR* szDelimiters = _T(" \t\r\n") )
    {
		tstring::size_type nPos = input_string.find_first_not_of(szDelimiters);
		if (tstring::npos != nPos)
		{
			input_string.erase(0, nPos);
		}
        return input_string;
    }

    // trim leading and trailing whitespace characters for the given tstring
    inline tstring& trim_all(tstring& input_string, const TCHAR* szDelimiters = _T(" \t\r\n") )
    {
		trim_left(input_string, szDelimiters);
		trim_right(input_string, szDelimiters);
        return input_string;
    }

	// convert tstring to std::string (MBCS and UNICODE)
	inline void to_char(const tstring& input_string, std::string& output_string)
	{
#if _UNICODE
		// first calculate the size we need to allocate
		int nOutSize = WideCharToMultiByte(
			CP_UTF8,			// code page
			0,					// performance and mapping flags
			input_string.c_str(),	// wide-character string
			input_string.length(),	// number of chars in string (-1 = the length will be calculated)
			NULL,				// buffer for new string
			0,					// size of buffer
			NULL,				// default char for invalid code page char
			NULL);				// set when default char used
		// now lets allocate string buffer and initialize it
		std::auto_ptr<char> spBuffer(new char[nOutSize+1]);
		memset(spBuffer.get(), NULL, sizeof(char)*(nOutSize+1) );
		// finally lets convert the wide char to multibyte
		nOutSize = WideCharToMultiByte(
			CP_UTF8,			// code page
			0,					// performance and mapping flags
			input_string.c_str(),	// wide-character string
			input_string.length(),	// number of chars in string (-1 = the length will be calculated)
			spBuffer.get(),		// buffer for new string
			nOutSize,			// size of buffer
			NULL,				// default char for invalid code page char
			NULL);				// set when default char used

		if ( nOutSize == 0 )
		{
			DWORD dwErr = ::GetLastError();
			return;
		}
		output_string = spBuffer.get();	
#else	// MBCS
		output_string = input_string;
#endif	// MBCS, UNICODE
	}

	// convert tstring to std::string (MBCS and UNICODE)
	inline void to_wchar(const tstring& input_string, std::wstring& output_string)
	{
#if _UNICODE
		output_string = input_string;
#else	// MBCS
		// first calculate the size we need to allocate
		int nOutSize = MultiByteToWideChar(
			CP_UTF8,			// code page
			0,					// character-type options
			input_string.c_str(),	// string to map
			static_cast<int>(input_string.size()),	// number of bytes in string
			NULL,				// wide-character buffer
			0);					// size of buffer
		// now lets allocate wstring buffer and initialize it
		std::auto_ptr<wchar_t> spBuffer(new wchar_t[nOutSize+1]);
		memset(spBuffer.get(), NULL, sizeof(wchar_t)*(nOutSize+1) );
		// finally lets convert the multibyte to wide char
		nOutSize = MultiByteToWideChar(
			CP_UTF8,			// code page
			0,					// character-type options
			input_string.c_str(),	// string to map
			static_cast<int>(input_string.size()),	// number of bytes in string
			spBuffer.get(),		// wide-character buffer
			nOutSize);			// size of buffer

		if ( nOutSize == 0 )
		{
			DWORD dwErr = ::GetLastError();
			return;
		}
		output_string = spBuffer.get ();
#endif	// MBCS, UNICODE
	}

};	// namespace tstring_helper

#endif //__TSTRING_HPP__