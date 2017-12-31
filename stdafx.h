// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>

// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


#if _MSC_VER > 1310
#pragma warning(disable:4351)
#endif /* _MSC_VER */

#include <cstdint>
typedef float float32;
typedef double float64;

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
