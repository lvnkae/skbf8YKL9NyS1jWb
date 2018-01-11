/*!
 *  @file   stock_holdings.h
 *  @brief  株保有銘柄
 *  @date   2018/01/09
 */
#pragma once

#include "stock_code.h"

#include "yymmdd.h"

namespace trading
{

/*!
 *  @brief  信用株保有銘柄(建玉)
 */
struct StockPosition
{
    StockCode m_code;       //!< 銘柄コード
    garnet::YYMMDD m_date;  //!< 建日
    float64 m_value;        //!< 建単価
    int32_t m_number;       //!< 数量
    bool m_b_sell;          //!< 売建フラグ

    StockPosition()
    : m_code()
    , m_date()
    , m_value(0.0)
    , m_number(0)
    , m_b_sell(false)
    {
    }
    StockPosition(uint32_t code,
                  const garnet::YYMMDD& date,
                  float64 value,
                  int32_t number,
                  bool b_sell)
    : m_code(code)
    , m_date(date)
    , m_value(value)
    , m_number(number)
    , m_b_sell(b_sell)
    {
    }

    /*!
     *  @note   比較
     *  @param  right
     *  @retval true    一致
     */
    bool operator==(const StockPosition& right) const;
};

} // namespace trading
