/*!
 *  @file   win_main_struct.cpp
 *  @brief  win_main内関数間でやりとりstatic変数をまとめる構造体
 *  @date   2017/05/08
 */

#include "win_main_struct.h"
#include "trade_assistor.h"

WinMainStruct::WinMainStruct()
: m_hInstance(nullptr)
, m_szTitle()
, m_szWindowClass()
, m_TradeAssistor()
{
}

WinMainStruct::~WinMainStruct()
{
}

