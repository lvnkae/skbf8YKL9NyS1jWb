/*!
 *  @file   stock_trading_command.cpp
 *  @brief  株取引命令
 *  @date   2017/12/27
 */
#pragma once

#include "stock_trading_command.h"

#include "stock_code.h"
#include "stock_trading_tactics_utility.h"
#include "trade_struct.h"
#include "trade_utility.h"

namespace trading
{

/*!
 *  @brief  StockOrderにパラメータ入れる
 *  @param[in]  investments 取引所種別
 *  @param[in]  code        銘柄コード
 *  @param[in]  order_type  注文種別
 *  @param[in]  order_cond  注文条件
 *  @param[in]  b_leverage  信用フラグ
 *  @param[in]  number      注文株数
 *  @param[in]  value       注文価格
 *  @param[out] o_order     格納先
 */
void SetOrderParam(eStockInvestmentsType investments,
                   const StockCode& code,
                   eOrderType order_type,
                   eOrderCondition order_cond,
                   bool b_leverage,
                   int32_t number,
                   float64 value,
                   StockOrder& o_order)
{
    o_order.m_code = code;
    o_order.m_number = number;
    o_order.m_value = value;
    o_order.m_b_leverage = b_leverage;
    o_order.m_b_market_order = trade_utility::is_market_order(value);
    o_order.m_type = order_type;
    o_order.m_condition = order_cond;
    o_order.m_investments = investments;
}

/*!
 *  @param  type            命令種別
 *  @param  tactics_id      戦略ID
 *  @param  target_group    対象グループ<戦略グループID>
 */
StockTradingCommand::StockTradingCommand(eCommandType type,
                                         const StockCode& code,
                                         int32_t tactics_id)
: m_type(type)
, m_code(code.GetCode())
, m_tactics_id(tactics_id)
{
}

/*!
 *  @param  type            命令種別
 *  @param  tactics_id      戦略ID
 *  @param  target_group    対象グループ<戦略グループID>
 */
StockTradingCommand_Order::StockTradingCommand_Order(eCommandType type,
                                                     const StockCode& code,
                                                     int32_t tactics_id,
                                                     int32_t group_id,
                                                     int32_t unique_id)
: StockTradingCommand(type, code, tactics_id)
, m_group_id(group_id)
, m_unique_id(unique_id)
, m_order()
{
}

/*!
 *  @brief  コマンド生成(緊急モード)
 *  @param  code            銘柄コード
 *  @param  tactics_id      戦略ID
 *  @param  target_group    対象グループ<戦略グループID>
 */
StockTradingCommand_Emergency::StockTradingCommand_Emergency(const StockCode& code,
                                                             int32_t tactics_id,
                                                             const std::unordered_set<int32_t>& target_group)
: StockTradingCommand(EMERGENCY, code, tactics_id)
, m_target_group(target_group)
{
}

/*!
 *  @brief  コマンド作成(発注[売買])
 *  @param  investments 取引所種別
 *  @param  type        命令種別
 *  @param  code        銘柄コード
 *  @param  tactics_id  戦略ID
 *  @param  group_id    戦略グループID
 *  @param  unique_id   戦略注文固有ID
 *  @param  order_type  注文種別(eOrderType)
 *  @param  order_cond  注文条件(eOrderCondition)
 *  @param  b_leverage  信用取引フラグ
 *  @param  number      注文株数
 *  @param  value       注文価格
 */
StockTradingCommand_BuySellOrder::StockTradingCommand_BuySellOrder(eStockInvestmentsType investments,
                                                                   const StockCode& code,
                                                                   int32_t tactics_id,
                                                                   int32_t group_id,
                                                                   int32_t unique_id,
                                                                   eOrderType order_type,
                                                                   eOrderCondition order_cond,
                                                                   bool b_leverage,
                                                                   int32_t number,
                                                                   float64 value)
: StockTradingCommand_Order(BUYSELL_ORDER, code, tactics_id, group_id, unique_id)
{
    SetOrderParam(investments, code, order_type, order_cond, b_leverage, number, value, m_order);
}

/*!
 *  @brief  コマンド作成(発注[信用返済])
 *  @param  investments 取引所種別
 *  @param  code        銘柄コード
 *  @param  tactics_id  戦略ID
 *  @param  group_id    戦略グループID
 *  @param  unique_id   戦略注文固有ID
 *  @param  order_type  注文種別(eOrderType)
 *  @param  order_cond  注文条件(eOrderCondition)
 *  @param  number      注文株数
 *  @param  value       注文価格
 *  @param  bg_date     建日
 *  @param  bg_value    建単価
 */
StockTradingCommand_RepLevOrder::StockTradingCommand_RepLevOrder(eStockInvestmentsType investments,
                                                                 const StockCode& code,
                                                                 int32_t tactics_id,
                                                                 int32_t group_id,
                                                                 int32_t unique_id,
                                                                 eOrderType order_type,
                                                                 eOrderCondition order_cond,
                                                                 int32_t number,
                                                                 float64 value,
                                                                 const garnet::YYMMDD& bg_date,
                                                                 float64 bg_value)
: StockTradingCommand_Order(REPAYMENT_LEV_ORDER, code, tactics_id, group_id, unique_id)
, m_bargain_date(bg_date)
, m_bargain_value(bg_value)
{
    SetOrderParam(investments, code, order_type, order_cond, true, number, value, m_order);
}

/*!
 *  @brief  コマンド作成(発注[訂正/取消])
 *  @param  src_command 売買命令(この命令で上書き(訂正)、またはこの命令を取消す)
 *  @param  order_type  命令種別
 *  @param  order_id    注文番号(証券会社が発行したもの)
 */
StockTradingCommand_ControllOrder::StockTradingCommand_ControllOrder(const StockTradingCommand& src_command,
                                                                     eOrderType order_type,
                                                                     int32_t order_id)
: StockTradingCommand_Order(CONTROL_ORDER,
                            src_command.GetCode(),
                            src_command.GetTacticsID(),
                            src_command.GetOrderGroupID(),
                            src_command.GetOrderUniqueID())
, m_order_id(order_id)
{
    // 株数以外をコピー
    CopyStockOrderWithoutNumber(src_command);
    m_order.m_type = order_type;
}

/*!
 *  @brief  戦略グループID取得
 */
int32_t StockTradingCommand::GetOrderGroupID() const
{
    return tactics_utility::BlankGroupID();
}
/*!
 *  @brief  戦略注文固有ID取得
 */
int32_t StockTradingCommand::GetOrderUniqueID() const
{
    return tactics_utility::BlankOrderUniqueID();
}
/*!
 *  @brief  株注文番号取得
 */
int32_t StockTradingCommand::GetOrderID() const
{
    return trade_utility::BlankOrderID();
}

/*!
 *  @brief  同属性の株注文命令か
 */
bool StockTradingCommand_Order::IsSameAttrOrder(const StockTradingCommand& right) const
{
    if (!right.IsOrder()) {
        return false;
    }
    return right.GetCode() == GetCode() &&
           right.GetTacticsID() == GetTacticsID() &&
           right.GetOrderGroupID() == m_group_id;
}
/*!
 *  @brief  rightは同属の売買注文か？
 *  @param  right   比較する命令
 */
bool StockTradingCommand_BuySellOrder::IsSameBuySellOrder(const StockTradingCommand& right) const
{
    if (right.GetType() != BUYSELL_ORDER) {
        return false;
    }
    const eOrderType order_type = m_order.m_type;
    const eOrderType r_order_type = right.GetOrderType();
    if (order_type != r_order_type) {
        return false;
    }
    return IsSameAttrOrder(right);
}

/*!
 *  @brief  numberを除いてStockOrderをCopy
 */
void StockTradingCommand_Order::CopyStockOrderWithoutNumber(const StockTradingCommand& src)
{
    const int32_t org_number = m_order.m_number;
    m_order = std::move(src.GetOrder());
    m_order.m_number = org_number;
}
/*!
 *  @brief  StockOrderのnumberを除いてCopy
 */
void StockTradingCommand_BuySellOrder::CopyBuySellOrder(const StockTradingCommand& src)
{
    if (src.GetType() != BUYSELL_ORDER) {
        return;
    }
    // 同属命令同士でのコピーに使うものなのでUniqueIDとStockOrder(株数以外)だけで良い
    m_unique_id = src.GetOrderUniqueID();
    CopyStockOrderWithoutNumber(src);
}

} // namespace trading
