/*!
 *  @file   trade_utility.h
 *  @brief  トレード関連utility関数
 *  @date   2018/01/05
 */
#pragma once

namespace trading
{
namespace trade_utility
{

/*!
 *  @brief  株価が同一か
 *  @param  src
 *  @param  dst
 *  @retval 同一とみなす
 *  @note   浮動小数点比較なので一致は範囲チェック
 */
bool same_value(float64 src, float64 dst);

/*!
 *  @brief  成行指定か
 *  @param  value   注文価格
 */
bool is_market_order(float64 value);

/*!
 *  @brief  株注文番号(証券会社発行)：空
 */
int32_t BlankOrderID();


} // namespace trade_utility
} // namespace trading
