/*!
 *  @file   stock_portfolio.h
 *  @brief  株監視銘柄データ
 *  @date   2017/12/24
 */
#pragma once

#include "hhmmss.h"
#include "stock_code.h"
#include <string>
#include <vector>

namespace std { struct tm; }
namespace trading
{
struct RcvStockValueData;

/*!
 *  @brief  株監視銘柄データ
 *  @note   時系列価格データ群がメインなのでポートフォリオとは言わんかも
 */
struct StockPortfolio
{
    struct stockValue
    {
        HHMMSS m_hhmmss;    //!< 時分秒
        float64 m_value;    //!< 価格
        int64_t m_number;   //!< 出来高

        stockValue()
        : m_hhmmss()
        , m_value(0.f)
        , m_number(0)
        {
        }

        stockValue(int32_t h, int32_t m, int32_t s)
        : m_hhmmss(h, m, s)
        , m_value(0.f)
        , m_number(0)
        {
        }
    };

    StockCode m_code;   //!< 銘柄コード
    float64 m_open;     //!< 始値
    float64 m_high;     //!< 高値
    float64 m_low;      //!< 安値
    float64 m_close;    //!< 前営業日終値
    std::vector<stockValue> m_value_data;   //!< 時系列価格データ群

    StockPortfolio()
    : m_code()
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_value_data()
    {
    }
    StockPortfolio(uint32_t scode)
    : m_code(scode)
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_value_data()
    {
    }

    /*!
     *  @brief  価格データ更新
     *  @param  src     価格データ
     *  @param  date    取得時刻(サーバ時間を使う)
     */
    void UpdateValueData(const RcvStockValueData& src, const std::tm& date);
};

/*!
 *  @brief  価格データ(1銘柄分)受信形式
 */
struct RcvStockValueData
{
    uint32_t m_code;    //!< 銘柄コード
    float64 m_value;    //!< 現値
    float64 m_open;     //!< 始値
    float64 m_high;     //!< 高値
    float64 m_low;      //!< 安値
    float64 m_close;    //!< 前営業日終値    
    int64_t m_number;   //!< 出来高

    RcvStockValueData()
    : m_code(0)
    , m_value(0.f)
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_number(0)
    {
    }
};

} // namespace trading

