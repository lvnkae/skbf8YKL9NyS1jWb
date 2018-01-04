/*!
 *  @file   stock_trading_tactics.cpp
 *  @brief  株取引戦略
 *  @date   2017/12/08
 */
#include "stock_trading_tactics.h"

#include "stock_portfolio.h"
#include "stock_trading_command.h"
#include "trade_assistant_setting.h"
#include "trade_define.h"

#include <algorithm>
#include <ctime>

namespace trading
{

/*!
 */
StockTradingTactics::StockTradingTactics()
: m_emergency()
, m_fresh()
, m_repayment()
{
}


/*!
 *  @brief  緊急モードを追加する
 *  @param  emergency   緊急モード設定
 */
void StockTradingTactics::AddEmergencyMode(const Emergency& emergency)
{
    m_emergency.emplace_back(emergency);
}
/*!
 *  @brief  新規注文を追加
 *  @param  order   発注設定
 */
void StockTradingTactics::AddFreshOrder(const Order& order)
{
    m_fresh.emplace_back(order);
}
/*!
 *  @brief  返済注文を追加
 *  @param  order   発注設定
 */
void StockTradingTactics::AddRepaymentOrder(const Order& order)
{
    m_repayment.emplace_back(order);
}



/*!
 *  @brief  判定
 *  @param  hhmmss      現在時分秒
 *  @param  valuedata   価格データ(1銘柄分)
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
bool StockTradingTactics::Trigger::Judge(const HHMMSS& hhmmss,
                                         const StockPortfolio& valuedata,
                                         TradeAssistantSetting& script_mng) const
{
    if (valuedata.m_value_data.empty()) {
        return false; // 価格データがなかったら判定しない
    }

    switch (m_type)
    {
    case VALUE_GAP:
        {
            if (static_cast<int32_t>(valuedata.m_close) <= 0) {
                // 0除算対策(error)
                // 上場初日だけは前日終値が存在しない(が大半はエラー)
                return false;
            }
            float64 v_high = 0.f;   // 期間高値
            float64 v_low = 0.f;    // 期間安値
            float64 v_open = 0.f;   // 期間始値
            // 指定期間[現在,指定秒前]の始値/高値/安値を得る
            {
                const int32_t pastsec = hhmmss.GetPastSecond();
                const int32_t rangesec = m_signed_param;
                const size_t presz = valuedata.m_value_data.size();
                //auto rit_end = valuedata.m_value_data.rend();
                int32_t rinx = 0;
                for (auto rit = valuedata.m_value_data.rbegin(); rit != valuedata.m_value_data.rend(); ++rit) {
                    if ((pastsec - rit->m_hhmmss.GetPastSecond()) <= rangesec) {
                        // 出来高なし(=データ先頭)なら前日終値を現値とする
                        const float64 v_now = (rit->m_number == 0) ?valuedata.m_close
                                                                   :rit->m_value;
                        v_high = std::max(v_high, v_now);
                        v_low  = (static_cast<int32_t>(v_low) == 0) ?v_now
                                                                    :std::min(v_low, v_now);
                        v_open = v_now;
                    }
                    rinx++;
                }
            }
            // 期間始値が存在する場合のみギャップを調べる
            if (v_open > 0.f) {
                if (m_float_param > 0.f) {
                    const float64 rateU = ((v_high-v_open)/v_open)*100.f;
                    return rateU >= m_float_param; 
                } else {
                    const float64 rateD = ((v_low-v_open)/v_open)*100.f;
                    return rateD <= m_float_param;
                }
            }
        }
        break;

    case NO_CONTRACT:
        {
            const int32_t pastsec = hhmmss.GetPastSecond();
            const int32_t latestsec = valuedata.m_value_data.back().m_hhmmss.GetPastSecond();
            return (pastsec - latestsec) >= m_signed_param;
        }
        break;

    case SCRIPT_FUNCTION:
        {
            const auto& latest = valuedata.m_value_data.back();
            // 出来高なしなら判定しない(データ先頭のみ時間記録用に存在し得る)
            if (latest.m_number > 0) {
                float64 lvalue = latest.m_value;
                return script_mng.CallJudgeFunction(m_signed_param,
                                                    lvalue,
                                                    valuedata.m_high,
                                                    valuedata.m_low,
                                                    valuedata.m_close);
            }
        }
    }

    return false;
}

/*!
 *  @brief  戦略解釈
 *  @param  hhmmss          現在時分秒
 *  @param  em_group        緊急モード対象グループ<戦略グループID>
 *  @param  valuedata       価格データ(1銘柄分)
 *  @param  script_mng      外部設定(スクリプト)管理者
 *  @param  enqueue_func    命令をキューに入れる関数
 */
void StockTradingTactics::Interpret(const HHMMSS& hhmmss,
                                    const std::unordered_set<int32_t>& em_group,
                                    const StockPortfolio& valuedata,
                                    TradeAssistantSetting& script_mng,
                                    const EnqueueFunc& enqueue_func) const
{
    const StockCode& s_code(valuedata.m_code);

    // 緊急モード判定
    for (const auto& emg: m_emergency) {
        if (!emg.Judge(hhmmss, valuedata, script_mng)) {
            continue;
        }
        StockTradingCommand emergency_command(s_code,
                                              m_unique_id,
                                              emg.RefTargetGroup());
        enqueue_func(emergency_command);
    }
    // 新規注文判定
    for (const auto& order: m_fresh) {
        if (em_group.end() != em_group.find(order.GetGroupID())) {
            continue; // 緊急モード制限中
        }
        if (!order.Judge(hhmmss, valuedata, script_mng)) {
            continue;
        }
        const auto& latest = valuedata.m_value_data.back();
        const float64 value = script_mng.CallGetValueFunction(order.GetValueFuncReference(),
                                                              latest.m_value,
                                                              valuedata.m_high,
                                                              valuedata.m_low,
                                                              valuedata.m_close);
        const trading::eOrderType otype = (order.GetType() == BUY) ?ORDER_BUY 
                                                                   :ORDER_SELL;
        StockTradingCommand order_command(s_code,
                                          m_unique_id,
                                          order.GetGroupID(),
                                          order.GetUniqueID(),
                                          static_cast<int32_t>(otype),
                                          static_cast<int32_t>(CONDITION_NONE), // >ToDo< 条件対応
                                          order.GetIsLeverage(),
                                          order.GetVolume(),
                                          value);
        enqueue_func(order_command);
    }
    // 返済注文判定
    for (const auto& order: m_repayment) {
#if 0
        if (b_emergency && !order.IsForEmergency()) {
            continue;
        }
        if (!order.Judge(hhmmss, valuedata, script_mng)) {
            continue;
        }
        const auto& latest = valuedata.m_value_data.back();
        const float64 value = script_mng.CallGetValueFunction(order.GetValueFuncReference(),
                                                              latest.m_value,
                                                              valuedata.m_high,
                                                              valuedata.m_low,
                                                              valuedata.m_close);
        StockTradingCommand order_command(StockTradingCommand::ORDER,
                                          s_code,
                                          valuedata.m_name,
                                          m_unique_id,
                                          order.GetGroupID());
        {
            trading::eOrderType otype = order.GetType() == BUY ?ORDER_BUY :ORDER_SELL;
            order_command.SetOrderParam(static_cast<int32_t>(otype),
                                        static_cast<int32_t>(CONDITION_NONE), // >ToDo< 条件対応
                                        static_cast<uint32_t>(order.GetVolume()),
                                        value,
                                        order.GetIsLeverage());
        }
        enqueue_func(m_unique_id, order_command);
#endif
    }
}


} // namespace trading
