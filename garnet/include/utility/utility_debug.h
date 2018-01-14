/*!
 *  @file   utility_debug.h
 *  @brief  [common]debug関連Utility関数
 *  @date   2017/12/31
 */
#pragma once

#include <string>

namespace garnet
{
namespace utility_debug
{

/*!
 *  @brief  [Debug]utf-16文字列をdebug出力する(イミディエイト)
 *  @param  str_u16
 */
void DebugOutput(const std::wstring& str_u16);
/*!
 *  @brief  [Debug]utf-8文字列をdebug出力する(イミディエイト)
 *  @param  str_u8
 */
void DebugOutput(const std::string& str_u8);

} // namespace utility_debug
} // namespace garnet
