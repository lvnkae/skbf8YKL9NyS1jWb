/*!
 *  @file   trade_container.h
 *  @brief  トレーディング関連コンテナ
 *  @date   2018/01/06
 */
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace trading
{

/*!
 *  @brief  株式銘柄コードコンテナ<銘柄コード>
 */
typedef std::unordered_set<uint32_t> StockCodeContainer;
/*!
 *  @brief  株式銘柄コンテナ<銘柄コード, 銘柄名(utf-16)>
 */
typedef std::unordered_map<uint32_t, std::wstring> StockBrandContainer;

/*!
 *  @brief  現物株コンテナ<銘柄コード, 数量>
 */
typedef std::unordered_map<uint32_t, int32_t> SpotTradingsStockContainer;
/*!
 *  @brief  株信用建玉コンテナ
 */
struct StockPosition;
typedef std::vector<StockPosition> StockPositionContainer;


} // namespace trading
