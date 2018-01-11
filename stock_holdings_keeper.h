/*!
 *  @file   stock_holdings_keeper.h
 *  @brief  株保有銘柄管理
 *  @date   2018/01/09
 */
#pragma once

#include "stock_trading_command_fwd.h"
#include "trade_container.h"

#include "yymmdd.h"

#include <memory>
#include <vector>

namespace garnet { struct YYMMDD; }
namespace trading
{
class StockCode;
struct StockOrder;
struct StockExecInfoAtOrder;

//! 発注済み信用返済売買注文<注文番号(表示用), 注文命令>
typedef std::unordered_map<int32_t, StockTradingCommandPtr> ServerRepLevOrder;

/*!
 *  @brief  保有銘柄管理クラス
 *  @note   現物保有株・信用建玉の管理
 */
class StockHoldingsKeeper
{
public:
    StockHoldingsKeeper();
    ~StockHoldingsKeeper();

    /*!
     *  @brief  保有銘柄更新
     *  @param  spot        現物保有株
     *  @param  position    信用保有株(建玉)
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position);
    /*!
     *  @brief  当日約定情報更新
     *  @param  rcv_info        受信した約定情報(1日分フル)
     *  @param  diff_info       約定情報(前回との差分)
     *  @param  sv_rep_order    約定情報(差分)と対応する発注済み信用返済売買注文
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                        const std::vector<StockExecInfoAtOrder>& diff_info,
                        const ServerRepLevOrder& sv_rep_order);

    /*!
     *  @brief  当日約定情報の差分を得る
     *  @param[in]  rcv_info    受信した約定情報(1日分フル)
     *  @param[out] diff_info   前回との差分(格納先)    
     */
    void GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                         std::vector<StockExecInfoAtOrder>& diff_info);

    /*!
     *  @brief  現物チェック
     *  @param  code    銘柄コード
     *  @param  number  必要株数
     *  @retval true    指定銘柄をnumber株以上現物保有している
     */
    bool CheckSpotTradingStock(const StockCode& code, int32_t number) const;
    /*!
     *  @brief  現物保有株数取得
     *  @param  code    銘柄コード
     *  @retval 0   保有してない
     */
    int32_t GetSpotTradingStockNumber(const StockCode& code) const;


    /*!
     *  @brief  建玉チェックA
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     *  @param  number  必要株数
     *  @retval true    指定建日/建単価のポジが存在、かつ株数がnumber以上である
     */
    bool CheckPosition(const StockCode& code,
                       const garnet::YYMMDD& date, float64 value, bool b_sell,
                       int32_t number) const;
    /*!
     *  @brief  建玉チェックB
     *  @param  code    銘柄コード
     *  @param  b_sell  売建フラグ
     *  @param  number  必要株数
     *  @retval true    指定銘柄を合計number株以上ポジってる
     *                  (b_sellが真なら売建、偽なら買建玉でチェックした結果)
     */
    bool CheckPosition(const StockCode& code, bool b_sell, int32_t number) const;
    /*!
     *  @brief  建玉チェックC
     *  @param  code    銘柄コード
     *  @param  pos_id  建玉ID群
     *  @return true    建玉が一つ以上残ってる
     */
    bool CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id);
    /*!
     *  @brief  信用保有株数取得
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     *  @retval 0   保有してない
     */
    int32_t GetPositionNumber(const StockCode& code,
                              const garnet::YYMMDD& date, float64 value, bool b_sell) const;
    /*!
     *  @brief  信用保有株取得
     *  @param  code    銘柄コード
     *  @param  b_sell  売建フラグ
     */
    std::vector<StockPosition> GetPosition(const StockCode& code, bool b_sell) const;
    /*!
     *  @brief  信用保有株固有IDを得る
     *  @param[in]  user_order_id   注文番号(表示用)
     *  @param[out] dst             格納先
     */
    void GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const;

private:
    StockHoldingsKeeper(const StockHoldingsKeeper&);
    StockHoldingsKeeper(const StockHoldingsKeeper&&);
    StockHoldingsKeeper& operator= (const StockHoldingsKeeper&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};


} // namespace trading
