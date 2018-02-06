/*!
 *  @file   stock_holdings_keeper.cpp
 *  @brief  株保有銘柄管理
 *  @date   2018/01/09
 */

#include "stock_holdings_keeper.h"
#include "stock_holdings.h"

#include "stock_trading_command.h"
#include "trade_struct.h"
#include "trade_utility.h"

#include "hhmmss.h"
#include "yymmdd.h"

#include <algorithm>
#include <unordered_map>

namespace trading
{

class StockHoldingsKeeper::PIMPL
{
private:
    /*!
     *  @brief  信用保有株(建玉)データ<銘柄コード, <建玉固有ID, 保有銘柄>>
     *  @note   固有IDは当日約定注文との紐付け用(前営業日以前の玉はIDなし)
     */
    typedef std::unordered_map<uint32_t, std::list<std::pair<int32_t, StockPosition>>> StockPositionData;
    /*!
     *  @brief  現物保有株データ<銘柄コード, 株数>
     *  @note   現物は銘柄と株数でのみ管理する
     *  @note   (買い付け日や単価を指定しての売買は出来ないので)
     */
    typedef SpotTradingsStockContainer SpotTradingStockData;

    //! 現物保有株
    SpotTradingStockData m_spot_data;
    //! 信用保有株(建玉)
    StockPositionData m_position_data;

    //! 建玉固有ID発行元
    uint32_t m_position_id_source;
    //! 当日約定注文<注文番号(表示用), 約定情報>
    std::unordered_map<int32_t, StockExecInfoAtOrder> m_today_exec_order;

    /*!
     *  @brief  建玉固有ID発行
     */
    uint32_t IssuePositionID()
    {
        return ++m_position_id_source;
    }

    /*!
     *  @brief  約定情報を現物保有株に反映
     *  @param  diff    約定情報(差分) 1注文分
     */
    void ReflectExecInfoToSpot(const StockExecInfoAtOrder& diff)
    {
        switch (diff.m_type)
        {
        case ORDER_BUY:
            for (const auto& ex: diff.m_exec) {
                m_spot_data[diff.m_code] += ex.m_number;
            }
            break;
        case ORDER_SELL:
            for (const auto& ex: diff.m_exec) {
                auto it = m_spot_data.find(diff.m_code);
                if (it == m_spot_data.end()) {
                    continue; // 何故か保持情報がない(error)
                }
                if (it->second <= ex.m_number) {
                    m_spot_data.erase(it);
                } else {
                    it->second -= ex.m_number;
                }
            }
            break;
        default:
            break; // 不正な約定(error)
        }
    }

    /*!
     *  @param  約定情報を信用保有株に反映
     *  @param  diff            約定情報(差分) 1注文分
     *  @param  sv_rep_order    約定情報(差分)と対応する発注済み信用返済売買注文
     */
    void ReflectExecInfoToPosition(const StockExecInfoAtOrder diff, 
                                   const ServerRepLevOrder& sv_rep_order)
    {
        const bool b_sell = (diff.m_type == ORDER_SELL || diff.m_type == ORDER_REPSELL);
        switch (diff.m_type)
        {
        case ORDER_BUY:
        case ORDER_SELL:
            // 新規売買
            for (const auto& ex: diff.m_exec) {
                AddPosition(diff.m_code, ex.m_date, ex.m_value, b_sell, ex.m_number);
            }
            break;
        case ORDER_REPBUY:
        case ORDER_REPSELL:
            // 返済売買
            {
                const auto itSv = sv_rep_order.find(diff.m_user_order_id);
                if (itSv == sv_rep_order.end()) {
                    return; // なぜか見つからない(error)
                }
                const StockTradingCommand& command(*itSv->second);
                garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                if (bg_date.empty()) {
                    return; // なぜか返済売買命令じゃない(error)
                }
                const float64 bg_value = command.GetRepLevBargainValue();
                for (const auto& ex: diff.m_exec) {
                    DecPosition(diff.m_code, bg_date, bg_value, b_sell, ex.m_number);
                }
            }
            break;
        default:
            break; // 不正な約定(error)
        }
    }

    /*!
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     *  @param  number  株数
     */
    void InsertPosition(uint32_t code,
                        const garnet::YYMMDD& date, float64 value, bool b_sell,
                        int32_t number)
    {
        if (number <= 0) {
            return; // 株数不正(error)
        }
        StockPosition pos(code, date, value, number, b_sell);
        m_position_data[code].emplace_back(IssuePositionID(), std::move(pos));
    }
    /*!
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     */
    void AddPosition(uint32_t code,
                     const garnet::YYMMDD& date, float64 value, bool b_sell,
                     int32_t number)
    {
        if (number <= 0) {
            return; // 株数不正(error)
        }
        const auto itPosDat = m_position_data.find(code);
        if (itPosDat == m_position_data.end()) {
            InsertPosition(code, date, value, b_sell, number);
            return;
        }
        auto& pos_list(itPosDat->second);
        auto it = std::find_if(pos_list.begin(),
                               pos_list.end(),
                               [&date, value, b_sell]
                               (const std::pair<int32_t, StockPosition>& pos_unit) {
            const StockPosition& pos(pos_unit.second);
            if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                return false;
            }
            return pos.m_date == date && trade_utility::same_value(pos.m_value, value);
        });
        if (it == pos_list.end()) {
            InsertPosition(code, date, value, b_sell, number);
        } else {
            it->second.m_number += number;
        }
    }
    /*!
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     */
    void DecPosition(uint32_t code,
                     const garnet::YYMMDD& date, float64 value, bool b_sell,
                     int32_t number)
    {
        if (number <= 0) {
            return; // 株数不正(error)
        }
        const auto itPosDat = m_position_data.find(code);
        if (itPosDat == m_position_data.end()) {
            return; // なぜか見つからない(error)
        }
        auto& pos_list(itPosDat->second);
        auto it = std::find_if(pos_list.begin(),
                               pos_list.end(),
                               [&date, value, b_sell]
                               (const std::pair<int32_t, StockPosition>& pos_unit) {
            const StockPosition& pos(pos_unit.second);
            if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                return false;
            }
            return pos.m_date == date && trade_utility::same_value(pos.m_value, value);
        });
        if (it == pos_list.end()) {
            return; // なぜか見つからない(error)
        } else {
            if (it->second.m_number > number) {
                it->second.m_number -= number;
            } else {
                // 返済し終えたので削除
                pos_list.erase(it);
            }
        }
    }

public:
    PIMPL()
    : m_spot_data()
    , m_position_data()
    , m_position_id_source(0)
    , m_today_exec_order()
    {
    }

    /*!
     *  @brief  現物保有株数取得
     *  @param  code    銘柄コード
     */
    int32_t GetSpotTradingStockNumber(const StockCode& code) const
    {
        const auto it = m_spot_data.find(code.GetCode());
        if (it == m_spot_data.end()) {
            return 0;
        }
        return it->second;
    }


    /*!
     *  @brief  建玉チェックB
     *  @param  code    銘柄コード
     *  @param  b_sell  売建フラグ
     *  @param  number  必要株数
     *  @return true    code/b_sellのポジ合算で株数がnumber以上ある
     */
    bool CheckPosition(const StockCode& code, bool b_sell, int32_t number) const
    {
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            int32_t sum_number = 0;
            const auto& pos_list(itPosDat->second);
            for (const auto& pos_unit: pos_list) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                sum_number += pos.m_number;
                if (sum_number >= number) {
                    return true;
                }
            }
        }
        return false;
    }
    /*!
     *  @brief  建玉チェックC
     *  @param  code    銘柄コード
     *  @param  pos_id  建玉ID群
     *  @return true    建玉が一つ以上残ってる
     */
    bool CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id)
    {
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            const auto& pos_list(itPosDat->second);
            for (int32_t id: pos_id) {
                const auto itPos
                    = std::find_if(pos_list.begin(),
                                   pos_list.end(),
                                   [id](const std::pair<int32_t, StockPosition>& src) {
                    return src.first == id;
                });
                if (itPos != pos_list.end()) {
                    return true;
                }
            }
        }
        return false;
    }
    /*!
     *  @brief  信用保有株数取得
     *  @param  code    銘柄コード
     *  @param  date    建日
     *  @param  value   建単価
     *  @param  b_sell  売建フラグ
     */
    int32_t GetPositionNumber(const StockCode& code,
                              const garnet::YYMMDD& date, float64 value, bool b_sell) const
    {
        if (!date.empty()) {
            const auto itPosDat = m_position_data.find(code.GetCode());
            if (itPosDat != m_position_data.end()) {
                const auto& pos_list(itPosDat->second);
                for (const auto& pos_unit: pos_list) {
                    const StockPosition& pos(pos_unit.second);
                    if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                        continue;
                    }
                    if (pos.m_date == date && trade_utility::same_value(pos.m_value, value)) {
                        return pos.m_number;
                    }
                }
            }
        }
        return 0;
    }
    /*!
     *  @brief  信用保有株取得
     *  @param  code    銘柄コード
     *  @param  b_sell  売建フラグ
     */
    std::vector<StockPosition> GetPosition(const StockCode& code, bool b_sell) const
    {
        std::vector<StockPosition> pos_vec;
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            const auto& pos_list(itPosDat->second);
            pos_vec.reserve(pos_list.size());
            for (const auto& pos_unit: pos_list) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                pos_vec.emplace_back(pos);
            }
        }
        return pos_vec;
    }
    /*!
     *  @brief  信用保有株固有IDを得る
     *  @param[in]  user_order_id   注文番号(表示用)
     *  @param[out] dst             格納先
     */
    void GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const
    {
        const auto it = m_today_exec_order.find(user_order_id);
        if (it == m_today_exec_order.end()) {
            return; // なぜか見つからない(error)
        }
        const StockExecInfoAtOrder& ex_info(it->second);
        const eOrderType odtype = ex_info.m_type;
        if (!ex_info.m_b_leverage) {
            return; // 現物は対象外
        }
        if (odtype != ORDER_SELL && odtype != ORDER_BUY) {
            return; // 新規売買のみ対象
        }
        const auto itPosDat = m_position_data.find(it->second.m_code);
        if (itPosDat == m_position_data.end()) {
            return; // なぜか指定銘柄コードの建玉がない(error)
        }
        const bool b_sell = odtype == ORDER_SELL;
        for (const auto& ex: ex_info.m_exec) {
            const garnet::YYMMDD& date(ex.m_date);
            const float64 value = ex.m_value;
            for (const auto& pos_unit: itPosDat->second) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                if (pos.m_date == date && trade_utility::same_value(pos.m_value, value)) {
                    dst.push_back(pos_unit.first);
                }
            }
        }
    }


    /*!
     *  @brief  保有銘柄更新
     *  @param  spot        現物保有株
     *  @param  position    信用保有株
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position)
    {
        // 現物は上書きでOK
        m_spot_data.swap(std::move(SpotTradingStockData(spot)));
        // 信用は固有IDを付加してから上書き
        StockPositionData pos_data;
        for (const auto& pos: position) {
            const uint32_t code = pos.m_code.GetCode();
            const auto& cpos_list = m_position_data[code];
            const auto it = std::find_if(cpos_list.begin(),
                                         cpos_list.end(),
                                         [&pos](const std::pair<int32_t, StockPosition>& pos_unit)
            {
                return (pos == pos_unit.second);
            });
            // 存在するポジならID継続、新規ならID発行
            const int32_t pos_id = (it != cpos_list.end()) ?it->first
                                                           :IssuePositionID();
            pos_data[code].emplace_back(pos_id, pos);
        }
        m_position_data.swap(pos_data);
    }

    /*!
     *  @brief  当日約定情報の差分を得る
     *  @param[in]  rcv_info    受信した約定情報(1日分フル)
     *  @param[out] diff_info   前回との差分(格納先)    
     */
    void GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                         std::vector<StockExecInfoAtOrder>& diff_info)
    {
        diff_info.reserve(rcv_info.size());
        for (const auto& rcv: rcv_info) {
            const auto it = m_today_exec_order.find(rcv.m_user_order_id);
            if (it == m_today_exec_order.end()) {
                // 注文番号単位で前回存在しなかったらまるごと登録
                diff_info.emplace_back(rcv);
            } else {
                // 前回より増えてたら差分だけ登録
                const size_t prev_num = it->second.m_exec.size();
                const size_t now_num = rcv.m_exec.size();
                if (now_num > prev_num) {
                    StockExecInfoAtOrder t_ex(static_cast<const StockExecInfoAtOrderHeader&>(rcv));
                    t_ex.m_exec.reserve(now_num-prev_num);
                    for (size_t inx = prev_num; inx < now_num; inx++) {
                        t_ex.m_exec.emplace_back(it->second.m_exec[inx]);
                    }
                    diff_info.emplace_back(std::move(t_ex));
                }
            }
        }
    }
    /*!
     *  @brief  当日約定情報更新
     *  @param  rcv_info        受信した約定情報(1日分フル)
     *  @param  diff_info       約定情報(前回との差分)
     *  @param  sv_rep_order    約定情報(差分)と対応する発注済み信用返済売買注文
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                        const std::vector<StockExecInfoAtOrder>& diff_info,
                        const ServerRepLevOrder& sv_rep_order)
    {
        // 差分を保有銘柄に反映
        for (const auto& diff: diff_info) {
            if (!diff.m_b_leverage) {
                // 現物
                ReflectExecInfoToSpot(diff);
            } else {
                // 信用
                ReflectExecInfoToPosition(diff, sv_rep_order);
            }
        }
        // 当日約定情報上書き
        for (const auto& rcv: rcv_info) {
            m_today_exec_order.emplace(rcv.m_user_order_id, rcv);
        }
    }
};

StockHoldingsKeeper::StockHoldingsKeeper()
: m_pImpl(new PIMPL())
{
}

StockHoldingsKeeper::~StockHoldingsKeeper()
{
}

/*!
 *  @brief  保有銘柄更新
 *  @param  spot        現物保有株
 *  @param  position    信用保有株(建玉)
 */
void StockHoldingsKeeper::UpdateHoldings(const SpotTradingsStockContainer& spot,
                                         const StockPositionContainer& position)
{
    m_pImpl->UpdateHoldings(spot, position);
}

/*!
 *  @brief  当日約定情報の差分を得る
 *  @param[in]  rcv_info    受信した約定情報(1日分フル)
 *  @param[out] diff_info   前回との差分(格納先)    
 */
void StockHoldingsKeeper::GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                                          std::vector<StockExecInfoAtOrder>& diff_info)
{
    m_pImpl->GetExecInfoDiff(rcv_info, diff_info);
}
/*!
 *  @brief  当日約定情報更新
 *  @param  rcv_info        受信した約定情報(1日分フル)
 *  @param  diff_info       約定情報(前回との差分)
 *  @param  sv_rep_order    約定情報(差分)と対応する発注済み信用返済売買注文
 */
void StockHoldingsKeeper::UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                                         const std::vector<StockExecInfoAtOrder>& diff_info,
                                         const ServerRepLevOrder& sv_rep_order)
{
    m_pImpl->UpdateExecInfo(rcv_info, diff_info, sv_rep_order);
}

/*!
 *  @brief  現物チェック
 *  @param  code    銘柄コード
 *  @param  number  必要株数
 */
bool StockHoldingsKeeper::CheckSpotTradingStock(const StockCode& code, int32_t number) const
{
    const int32_t have_num = m_pImpl->GetSpotTradingStockNumber(code);
    return have_num >= number;
}
/*!
 *  @brief  現物保有株数取得
 *  @param  code    銘柄コード
 */
int32_t StockHoldingsKeeper::GetSpotTradingStockNumber(const StockCode& code) const
{
    return m_pImpl->GetSpotTradingStockNumber(code);
}

/*!
 *  @brief  建玉チェックA
 *  @param  code    銘柄コード
 *  @param  date    建日
 *  @param  b_sell  売建フラグ
 *  @param  value   建単価
 *  @param  number  必要株数
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code,
                                        const garnet::YYMMDD& date, float64 value, bool b_sell,
                                        int32_t number) const
{
    return m_pImpl->GetPositionNumber(code, date, value, b_sell) >= number;
}
/*!
 *  @brief  建玉チェックB
 *  @param  code    銘柄コード
 *  @param  b_sell  売建フラグ
 *  @param  number  必要株数
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code, bool b_sell, int32_t number) const
{
    return m_pImpl->CheckPosition(code, b_sell, number);
}
/*!
 *  @brief  建玉チェックC
 *  @param  code    銘柄コード
 *  @param  pos_id  建玉ID群
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id)
{
    return m_pImpl->CheckPosition(code, pos_id);
}
/*!
 *  @brief  信用保有株数取得
 *  @param  code    銘柄コード
 *  @param  date    建日
 *  @param  value   建単価
 *  @param  b_sell  売建フラグ
 */
int32_t StockHoldingsKeeper::GetPositionNumber(const StockCode& code,
                                               const garnet::YYMMDD& date,
                                               float64 value,
                                               bool b_sell) const
{
    return m_pImpl->GetPositionNumber(code, date, value, b_sell);
}
/*!
 *  @brief  信用保有株取得
 *  @param  code    銘柄コード
 *  @param  b_sell  売建フラグ
 */
std::vector<StockPosition> StockHoldingsKeeper::GetPosition(const StockCode& code,
                                                            bool b_sell) const
{
    return m_pImpl->GetPosition(code, b_sell);
}
/*!
 *  @brief  信用保有株固有IDを得る
 *  @param[in]  user_order_id   注文番号(表示用)
 *  @param[out] dst             格納先
 */
void StockHoldingsKeeper::GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const
{
    return m_pImpl->GetPositionID(user_order_id, dst);
}

/*!
 *  @note   信用建玉比較
 *  @param  right
 *  @retval true    一致
 */
bool StockPosition::operator==(const StockPosition& right) const
{
    // 銘柄コード・建日・建単価・売買種別まで一致したらOK
    // (保有数は見ない)
    return (m_code.GetCode() == right.m_code.GetCode() &&
            m_date == right.m_date &&
            trade_utility::same_value(m_value, right.m_value) &&
            ((m_b_sell && right.m_b_sell) || (!m_b_sell && !right.m_b_sell)));
}

} // namespace trading
