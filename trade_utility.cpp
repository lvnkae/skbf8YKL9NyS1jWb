/*!
 *  @file   trade_utility.cpp
 *  @brief  トレード関連utility関数
 *  @date   2018/01/05
 */
#include "trade_utility.h"

namespace trading
{
namespace trade_utility
{

/*!
 *  @brief  株価が同一か
 *  @param  src
 *  @param  dst
 */
bool same_value(float64 src, float64 dst)
{
    // 小数点第一位までは有効株価なのでより下位で判定
    const float64 diff = src - dst;
    return (-0.05 < diff && diff < 0.05);
}

/*!
 *  @brief  成行指定か
 *  @param  value   注文価格
 */
bool is_market_order(float64 value)
{
    // 0未満なら成行き指定(スクリプトでは-1指定)
    return value < 0.0;
}

/*!
 *  @brief  株注文番号(証券会社発行)：空
 */
int32_t BlankOrderID() { return -1; }
/*!
 *  @brief  株価小数点有効桁数：1
 */
int32_t ValueOrder() { return 1; }

} // namespace trade_utility
} // namespace trading
