/*!
 *  @file   stock_portfolio.h
 *  @brief  株監視銘柄データ
 *  @date   2017/12/24
 */
#pragma once

#include "stock_code.h"
#include "hhmmss.h"
#include "yymmdd.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace trading
{
struct RcvStockValueData;

/*!
 *  @brief  株式価格データ
 *  @note   任意期間の価格と出来高を集積したもの
 */
struct StockValueData
{
    struct stockValue
    {
        garnet::HHMMSS m_hhmmss;//!< 時分秒
        float64 m_value;        //!< 価格
        int64_t m_volume;       //!< 出来高

        stockValue()
        : m_hhmmss()
        , m_value(0.f)
        , m_volume(0)
        {
        }

        stockValue(const garnet::sTime& tm, float64 value, int64_t volume)
        : m_hhmmss(tm)
        , m_value(value)
        , m_volume(volume)
        {
        }
    };

    StockCode m_code;   //!< 銘柄コード
    float64 m_open;     //!< 始値
    float64 m_high;     //!< 高値
    float64 m_low;      //!< 安値
    float64 m_close;    //!< 前営業日終値
    std::vector<stockValue> m_value_data;   //!< 時系列価格データ群

    StockValueData()
    : m_code()
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_value_data()
    {
    }
    StockValueData(uint32_t scode)
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
    void UpdateValueData(const RcvStockValueData& src, const garnet::sTime& date);
};

/*!
 *  @brief  価格データ(1銘柄分)受信形式
 *  @note   ある瞬間の株データ
 */
struct RcvStockValueData
{
    uint32_t m_code;    //!< 銘柄コード
    float64 m_value;    //!< 現値
    float64 m_open;     //!< 始値
    float64 m_high;     //!< 高値
    float64 m_low;      //!< 安値
    float64 m_close;    //!< 前営業日終値    
    int64_t m_volume;   //!< 出来高

    RcvStockValueData()
    : m_code(0)
    , m_value(0.f)
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_volume(0)
    {
    }
};

} // namespace trading
