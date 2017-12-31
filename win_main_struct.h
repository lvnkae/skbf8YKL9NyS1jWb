/*!
 *  @file   win_main_struct.h
 *  @brief  win_main内関数間でやりとりstatic変数をまとめる構造体
 *  @note   WinMainクラスを作って中で完結させるのが正解？
 *  @date   2017/05/08
 */
#pragma once

#include <memory>

namespace trading { class TradeAssistor; }

struct WinMainStruct
{
    enum {
        MAX_LOADSTRING = 100
    };

    HINSTANCE m_hInstance;  				//!< 現在のインターフェイス
    TCHAR m_szTitle[MAX_LOADSTRING];		//!< タイトル バーのテキスト
    TCHAR m_szWindowClass[MAX_LOADSTRING];  //!< メイン ウィンドウ クラス名

    std::weak_ptr<trading::TradeAssistor>   m_TradeAssistor;    //!< トレード補助

    WinMainStruct();
    ~WinMainStruct();
};
