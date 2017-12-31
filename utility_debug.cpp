/*!
 *  @file   utility_debug.h
 *  @brief  [common]debug関連Utility関数
 *  @date   2017/12/31
 */
#include "utility_debug.h"

#if defined(_DEBUG)
#include <codecvt>
#include <iostream>
#endif/* defined(_DEBUG */

namespace utility
{

/*!
 *  @brief  [Debug]utf-16文字列をdebug出力する(イミディエイト)
 *  @param  str_u16
 */
void DebugOutput(const std::wstring& str_u16)
{
#if defined(_DEBUG)
    std::wcout << str_u16.c_str();
#endif/* _DEBUG */
}

/*!
 *  @brief  [Debug]utf-8文字列をdebug出力する(イミディエイト)
 *  @param  str_u8
 */
void DebugOutput(const std::string& str_u8)
{
#if defined(_DEBUG)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
    DebugOutput(utfconv.from_bytes(str_u8));
#endif/* _DEBUG */
}

} // namespace utility
