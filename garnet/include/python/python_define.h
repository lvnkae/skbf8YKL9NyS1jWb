/*!
 *  @file   python_define.h
 *  @brief  [common]python定義
 *  @date   2018/01/14
 */
#pragma once

// pythonバージョン(2.7 => 2700)
#define _PYTHON_VER 2700

#if (2000 <= _PYTHON_VER && _PYTHON_VER < 3000)
#elif (3000 <= _PYTHON_VER && _PYTHON_VER < 4000)
#define PYTHON_USE_WCHAR
#else
#error
#endif /* _PYTHON_VER */

#if defined(PYTHON_USE_WCHAR)
typedef wchar_t PYCHAR;
#define PYSTRING std::wstring
#else
typedef char PYCHAR;
#define PYSTRING std::string
#endif/* defined(PYTHON_USE_WCHAR) */
