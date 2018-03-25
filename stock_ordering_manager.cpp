/*!
 *  @file   stock_ordering_manager.cpp
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#include "stock_ordering_manager.h"

#include "securities_session.h"
#include "stock_holdings_keeper.h"
#include "stock_holdings.h"
#include "stock_portfolio.h"
#include "stock_trading_command_fwd.h"
#include "stock_trading_command.h"
#include "stock_trading_tactics.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"
#include "trade_utility.h"
#include "update_message.h"

#include "cipher_aes.h"
#include "garnet_time.h"
#include "yymmdd.h"
#include "twitter/twitter_session.h"
#include "utility/utility_datetime.h"
#include "utility/utility_string.h"

#include <list>
#include <thread>

namespace trading
{

namespace
{
void AddErrorMsg(const std::wstring& err, std::wstring& dst)
{
    if (dst.empty()) {
        dst = err;
    } else {
        dst += garnet::twitter::GetNewlineString() + err;
    }
}
std::wstring GetErrorMsgHeader()
{
    const std::wstring nl(std::move(garnet::twitter::GetNewlineString()));
    return nl + L"[error]" + nl;
}
} // namespace

class StockOrderingManager::PIMPL
{
private:
    /*!
     *  @brief  戦略識別情報
     *  @note   aとbが一致すれば同属、cが一致したら同一の注文
     */
    struct TacticsIdentifier
    {
        int32_t m_tactics_id;   //!< 戦略ID...(a)
        int32_t m_group_id;     //!< 戦略グループID...(b)
        int32_t m_unique_id;    //!< 戦略注文固有ID...(c)

        TacticsIdentifier(const StockTradingCommand& command)
        : m_tactics_id(command.GetTacticsID())
        , m_group_id(command.GetOrderGroupID())
        , m_unique_id(command.GetOrderUniqueID())
        {
        }
    };

    /*!
     *  @brief  緊急モード状態
     */
    struct EmergencyModeState
    {
        uint32_t m_code;
        int32_t m_tactics_id;
        std::unordered_set<int32_t> m_group;

        int64_t m_timer; //! 残り時間

        EmergencyModeState(uint32_t code,
                           int32_t tactics_id,
                           const std::unordered_set<int32_t>& group,
                           int64_t timer)
        : m_code(code)
        , m_tactics_id(tactics_id)
        , m_group(group)
        , m_timer(timer)
        {
        }

        void AddGroupID(const std::unordered_set<int32_t>& group)
        {
            for (int32_t group_id: group) {
                m_group.insert(group_id);
            }
        }
    };

    //! 証券会社とのセッション
    SecuritiesSessionPtr m_pSecSession;
    //! twitterとのセッション
    garnet::TwitterSessionForAuthorPtr m_pTwSession;

    //! 取引戦略データ<戦略ID, 戦略データ>
    std::unordered_map<int32_t, StockTradingTactics> m_tactics;
    //! 戦略データ紐付け情報<銘柄コード, 戦略ID>
    std::vector<std::pair<uint32_t, int32_t>> m_tactics_link;
    //! 緊急モード期間[ミリ秒] ※外部設定から取得
    const int64_t m_emergency_time_ms;
    //! 監視銘柄
    StockBrandContainer m_monitoring_brand;
    //! 監視銘柄データ<取引所種別, <銘柄コード, 1銘柄分の価格データ>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<uint32_t, StockValueData>> m_monitoring_data;
    //! 保有銘柄管理
    StockHoldingsKeeper m_holdings;

    //! 約定情報受信まで発注処理ロックする
    bool m_b_lock_odmng_and_wait_execinfo;

    //! 命令リスト
    std::list<StockTradingCommandPtr> m_command_list;
    //! 緊急モード状態
    std::list<EmergencyModeState> m_emergency_state;
    //! 結果待ち注文 ※要素数は1か0/注文は1つずつ処理する
    std::vector<StockTradingCommandPtr> m_wait_order;
    //! 発注済み注文<取引所種別, <注文番号(管理用), 命令>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<int32_t, StockTradingCommandPtr>> m_server_order;
    //! 当日約定注文<銘柄コード, <約定注文識別>> ※現物買/新規信用買/新規信用売が対象、再発注制御に使う
    typedef std::pair<TacticsIdentifier, std::vector<int32_t>> StockExecOrderIdentifier; // 約定注文識別<識別情報, 建玉固有ID群>
    std::unordered_map<uint32_t, std::list<StockExecOrderIdentifier>> m_exec_order;
    //! 注文番号対応表<表示用, 管理用>
    std::unordered_map<int32_t, int32_t> m_server_order_id;

    //! 現在の対象取引所
    eStockInvestmentsType m_investments;
    //! 経過時間[ミリ秒]
    int64_t m_tick_count;
    //! 最終発注応答時間(tick)
    int64_t m_last_tick_rcv_rep_order;

    /*!
     *  @brief  注文約定メッセージ出力
     *  @param  command_ptr 注文命令
     *  @param  exec_info   約定情報(1注文分)
     *  @param  date        発生日時
     *  @note   出力先はtwitter
     */
    void OutputExecMessage(const StockTradingCommandPtr& command_ptr,
                           const StockExecInfoAtOrder& ex_info)
    {
        const int32_t order_id = ex_info.m_user_order_id;
        const StockOrder order(std::move(command_ptr->GetOrder()));
        const std::wstring name(m_monitoring_brand[order.GetCode()]);
        const std::wstring zone(L"JST");
        for (const auto& ex: ex_info.m_exec) {
            std::wstring src(L"約定");
            order.BuildMessageString(order_id, name, ex.m_number, ex.m_value, src);
            const std::wstring date(
                std::move(garnet::utility_datetime::ToRFC1123(ex.m_date, ex.m_time, zone)));
            m_pTwSession->Tweet(date, src);
        }
    }

    /*!
     *  @brief  発注コールバック
     *  @param  b_result    成否
     *  @param  rcv_order   注文結果
     *  @param  sv_date     サーバ時刻
     *  @param  investments 発注時の取引所種別
     *  @note   通信遅延で跨ぐ可能性があるので現在(this)のinvestmentsは使わない
     */
    void StockOrderCallback(bool b_result,
                            const RcvResponseStockOrder& rcv_order,
                            const std::wstring& sv_date,
                            eStockInvestmentsType investments)
    {
        std::wstring message((b_result) ?L"注文受付" : L"注文失敗");
        // 発注応答時間(tick)更新
        m_last_tick_rcv_rep_order = garnet::utility_datetime::GetTickCountGeneral();
        //
        if (m_wait_order.empty()) {
            // なぜか注文待ちがない(error)
            message += GetErrorMsgHeader() + L"%wait_order is empty";
            m_pTwSession->Tweet(sv_date, message);
        } else {
            const StockTradingCommandPtr& w_cmd_ptr = m_wait_order.front();
            const StockOrder w_order(std::move(w_cmd_ptr->GetOrder()));
            //
            std::wstring err_msg;
            if (b_result) {
                if (w_order != rcv_order) {
                    // 受付と待ちが食い違ってる(error)
                    err_msg = L"isn't equal %wait_order and %rcv_order";
                }
                switch (w_order.m_type)
                {
                case ORDER_BUY:
                case ORDER_SELL:
                case ORDER_REPSELL:
                case ORDER_REPBUY:
                    m_server_order[investments].emplace(rcv_order.m_order_id, w_cmd_ptr);
                    m_server_order_id.emplace(rcv_order.m_user_order_id, rcv_order.m_order_id);
                    break;
                case ORDER_CORRECT:
                    {
                        const auto itID = m_server_order_id.find(rcv_order.m_user_order_id);
                        if (itID != m_server_order_id.end()) {
                            auto& sv_order(m_server_order[investments]);
                            auto itSvOrder = sv_order.find(itID->second);
                            if (itSvOrder == sv_order.end()) {
                                AddErrorMsg(L"not found %server_order", err_msg);
                            } else {
                                // 価格上書き
                                itSvOrder->second->SetOrderValue(w_order.m_value);
                            }
                        } else {
                            AddErrorMsg(L"fail to cnv %order_id from %user_order_id", err_msg);
                        }
                    }
                    break;
                case ORDER_CANCEL:
                    {
                        const auto itID = m_server_order_id.find(rcv_order.m_user_order_id);
                        if (itID != m_server_order_id.end()) {
                            // 削除
                            if (0 == m_server_order[investments].erase(itID->second)) {
                                AddErrorMsg(L"fail to erase %server_order", err_msg);
                            }
                        } else {
                            AddErrorMsg(L"fail to cnv %order_id from %user_order_id", err_msg);
                        }
                    }
                    break;
                default:
                    break;
                }
            }
            // 通知
            {
                if (w_order.m_type == ORDER_NONE) {
                    // orderが空で呼ばれることもある(発注失敗時)
                } else {
                    const std::wstring name(m_monitoring_brand[w_order.GetCode()]);
                    w_order.BuildMessageString(rcv_order.m_user_order_id,
                                               name, w_order.m_number, w_order.m_value,
                                               message);
                }
                if (!err_msg.empty()) {
                    message += GetErrorMsgHeader() + err_msg;
                }
                m_pTwSession->Tweet(sv_date, message);
            }
            // 注文待ち解除(成否問わない)
            m_wait_order.pop_back();
            // 失敗してたら次の約定情報取得まで発注処理をロックする
            if (!b_result) {
                m_b_lock_odmng_and_wait_execinfo = true;
            }
        }
    }

    /*!
     *  @brief  緊急モード状態登録
     *  @param  command     命令
     */
    void EntryEmergencyState(const StockTradingCommand& command)
    {
        const uint32_t code = command.GetCode();
        const int32_t tactics_id = command.GetTacticsID();
        const std::unordered_set<int32_t> em_group(std::move(command.GetEmergencyTargetGroup()));
        for (auto& emstat: m_emergency_state) {
            if (emstat.m_code == code && emstat.m_tactics_id == tactics_id) {
                // すでにあれば更新
                emstat.AddGroupID(em_group);
                emstat.m_timer = m_emergency_time_ms;
                return;
            }
        }
        m_emergency_state.emplace_back(code, tactics_id, em_group, m_emergency_time_ms);
    }

    /*!
     *  @brief  発注取消
     *  @param  command     取消命令
     *  @param  investments 取引所種別
     */
    void CancelOrderCommand(const StockTradingCommand& command, eStockInvestmentsType investments)
    {
        const std::unordered_set<int32_t> em_group(std::move(command.GetEmergencyTargetGroup()));
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();

        // 発注前注文取消
        {
            auto itRmv = std::remove_if(m_command_list.begin(),
                                        m_command_list.end(),
                                        [&command, &em_group](const StockTradingCommandPtr& dst)
            {
                const StockTradingCommand& dst_command(*dst);
                if (!dst_command.IsOrder()) {
                    return false; // 発注命令じゃない
                }
                if (command.GetCode() != dst_command.GetCode() ||
                    command.GetTacticsID() != dst_command.GetTacticsID()) {
                    return false; // 別銘柄または別戦略
                }
                if (ORDER_CANCEL == dst_command.GetOrderType()) {
                    return false; // 取消命令は取り消さない
                }
                const int32_t group_id = dst_command.GetOrderGroupID();
                for (const int32_t em_gid: em_group) {
                    if (group_id == em_gid) {
                        return true;
                    }
                }
                return false;
            });
            if (m_command_list.end() != itRmv) {
                m_command_list.erase(itRmv, m_command_list.end());
            }
        }

        // 発注済み注文取消
        for (const auto& sv_order: m_server_order[investments]) {
            const StockTradingCommand& sv_cmd(*sv_order.second);
            const int32_t sv_tactics_id = sv_cmd.GetTacticsID();
            if (sv_tactics_id == tactics_id && sv_cmd.GetCode() == code) {
                const int32_t sv_order_id = sv_order.first;
                auto itCr = std::find_if(m_command_list.begin(),
                                            m_command_list.end(),
                                            [sv_order_id](const StockTradingCommandPtr& dst) {
                    return dst->GetOrderType() == ORDER_CANCEL &&
                            dst->GetOrderID() == sv_order_id;
                });
                if (itCr != m_command_list.end()) {
                    continue; // もう積んである
                }
                const int32_t sv_group_id = sv_cmd.GetOrderGroupID();
                auto itEm = std::find(em_group.begin(), em_group.end(), sv_group_id);
                if (itEm != em_group.end()) {
                    // 注文取消を先頭に積む
                    m_command_list.emplace_front(
                        new StockTradingCommand_ControllOrder(sv_cmd,
                                                              ORDER_CANCEL,
                                                              sv_order_id));
                }
            }
        }
    }

    /*!
     *  @brief  発注済み注文チェック
     *  @param  command     出そうとしてる命令
     *  @param  investments 取引所種別
     *  @note   同属注文があったらtrueを返す
     *  @note     同一 -> 弾く
     *  @note     同属 -> commandが下位 -> 弾く
     *  @noteq         -> commandが上位 -> 価格訂正
     */
    bool CheckServerOrder(const StockTradingCommand& command,
                          eStockInvestmentsType investments)
    {
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();
        const int32_t tac_uqid = command.GetOrderUniqueID();

        bool b_command_reject = false;

        // 発注済み注文チェック
        for (const auto& sv_order: m_server_order[investments]) {
            const StockTradingCommand& sv_cmd(*sv_order.second);
            if (command.IsSameAttrOrder(sv_cmd)) {
                b_command_reject = true;
                if (sv_cmd.GetOrderUniqueID() >= tac_uqid) {
                    // 同一、または同属上位注文発注済みなので無視
                    continue;
                }
                // 同属下位命令発注済みのため価格訂正を末尾に積む
                m_command_list.emplace_back(
                    new StockTradingCommand_ControllOrder(command,
                                                          ORDER_CORRECT,
                                                          sv_order.first));
            }
        }
        // 発注待機注文チェック
        for (auto& lcommand: m_command_list) {
            if (lcommand->IsSameBuySellOrder(command)) {
                b_command_reject = true;
                if (command.GetOrderUniqueID() > lcommand->GetOrderUniqueID()) {
                    // 上書き(後勝ち)
                    lcommand->CopyBuySellOrder(command);
                }
            }
        }

        return b_command_reject;
    }

    /*!
     *  @brief  取引命令登録
     *  @param  command     命令
     *  @param  investments 取引所種別
     */
    void EntryCommand(const StockTradingCommandPtr& command_ptr, 
                      eStockInvestmentsType investments)
    {
        if (m_b_lock_odmng_and_wait_execinfo) {
            return;
        }

        StockTradingCommand& command(*command_ptr);
        const int32_t tactics_id = command.GetTacticsID();
        const uint32_t code = command.GetCode();

        // 命令種別ごとの処理
        switch (command.GetType())
        {
        case StockTradingCommand::EMERGENCY:
            // 緊急モード状態登録
            EntryEmergencyState(command);
            // 注文取消(中で命令積んでる)
            CancelOrderCommand(command, investments);
            break;

        case StockTradingCommand::REPAYMENT_LEV_ORDER:
        case StockTradingCommand::BUYSELL_ORDER:
            {
                // 発注結果待ち注文チェック
                if (!m_wait_order.empty()) {
                    const StockTradingCommand& w_cmd = *m_wait_order.front();
                    if (command.IsSameAttrOrder(*m_wait_order.front())) {
                        // 同属性注文待ちしてるので弾く(発注完了後に対処する)
                        return;
                    }
                }
                // 発注済み注文チェック(&価格訂正)
                if (CheckServerOrder(command, investments)) {
                    return; // 先行注文に敗けたか訂正したので打ち切り
                }

                const eOrderType odtype = command.GetOrderType();
                const bool b_leverage = command.IsLeverageOrder();

                // 当日約定注文チェック(新規売買注文のみ)
                if (odtype == ORDER_BUY || (odtype == ORDER_SELL && b_leverage)) {
                    for (const auto& ex_order: m_exec_order[code]) {
                        const TacticsIdentifier& tc_id(ex_order.first);
                        if (command.GetTacticsID() != tc_id.m_tactics_id ||
                            command.GetOrderGroupID() != tc_id.m_group_id) {
                            continue;
                        }
                        if (!b_leverage) {
                            // 現物は再注文不可(差金決済が面倒くさいので)
                            return;
                        }
                        if (m_holdings.CheckPosition(code, ex_order.second)) {
                            // 前回注文時の建玉が残ってる
                            return;
                        }
                    }
                }

                // 保有銘柄チェック・全株指定展開・建日無指定展開
                if (odtype == ORDER_REPBUY || odtype == ORDER_REPSELL) {
                    // 信用返済売買
                    const bool b_sell = (odtype == ORDER_REPBUY); // 返買ならば売建玉を調べる
                    const garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                    if (!bg_date.empty()) {
                        // 建玉指定返済
                        const float64 bg_value = command.GetRepLevBargainValue();
                        const int32_t have_num
                            = m_holdings.GetPositionNumber(code, bg_date, bg_value, b_sell);
                        if (have_num <= 0) {
                            return; // 保有してない
                        }
                        const int32_t req_num = command.GetOrderNumber();
                        if (req_num < 0) {
                            // 全株指定
                            command.SetOrderNumber(have_num);
                        } else if (have_num < req_num) {
                            return; // 足りない
                        }
                    } else {
                        // 株数指定返済
                        bool b_add_command = false;
                        int32_t req_num = command.GetOrderNumber();
                        if (!m_holdings.CheckPosition(code, b_sell, req_num)) {
                            return; // 足りない
                        }
                        const auto pos_list(std::move(m_holdings.GetPosition(code, b_sell)));
                        if (pos_list.empty()) {
                            return; // 足りてることを確認した後なのに取得できない(error)
                        }
                        const StockOrder order(std::move(command.GetOrder()));
                        for (const auto& pos: pos_list) {
                            int32_t order_num = 0;
                            if (req_num < 0) {
                                // 全株指定
                                order_num = pos.m_number;
                            } else if (req_num > pos.m_number) {
                                // 要求数が建玉より多い
                                order_num = pos.m_number;
                                req_num -= pos.m_number;
                            } else {
                                // 要求数を賄いきった
                                order_num = req_num;
                                req_num = 0;
                            }
                            if (!b_add_command) {
                                // 初回は今の命令を使用
                                command.SetOrderNumber(order_num);
                                command.SetRepLevBargain(pos.m_date, pos.m_value);
                                b_add_command = true;
                            } else {
                                // 建玉を跨ぐ分は追加コマンド(先頭に積む)
                                m_command_list.emplace_front(
                                    new StockTradingCommand_RepLevOrder(investments,
                                                                        code,
                                                                        tactics_id,
                                                                        command.GetOrderGroupID(),
                                                                        command.GetOrderUniqueID(),
                                                                        odtype,
                                                                        order.m_condition,
                                                                        order_num,
                                                                        order.m_value,
                                                                        pos.m_date,
                                                                        pos.m_value));
                            }
                            if (req_num == 0) {
                                break;
                            }
                        }
                    }
                } else if (odtype == ORDER_SELL && !b_leverage) {
                    // 現物売
                    const int32_t have_num = m_holdings.GetSpotTradingStockNumber(code);
                    if (have_num <= 0) {
                        return; // 保有してない
                    }
                    const int32_t req_num = command.GetOrderNumber();
                    if (req_num < 0) {
                        // 全株指定
                        command.SetOrderNumber(have_num);
                    } else if (have_num < req_num) {
                        return; // 足りない
                    }
                }
            }
            // 末尾に積む
            m_command_list.emplace_back(command_ptr);
            break;

        default:
            break;
        }
    }

    /*!
     *  @brief  戦略解釈
     *  @param  investments 現在取引所種別
     *  @param  now_time    現在時分秒
     *  @param  sec_time    現セクション開始時刻
     *  @param  valuedata   価格データ(1取引所分)
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void InterpretTactics(eStockInvestmentsType investments,
                          const garnet::HHMMSS& now_time,
                          const garnet::HHMMSS& sec_time,
                          const std::unordered_map<uint32_t, StockValueData>& valuedata,
                          TradeAssistantSetting& script_mng)
    {
        std::unordered_set<int32_t> blank_group;
        for (const auto& link: m_tactics_link) {
            const uint32_t code = link.first;
            const int32_t tactics_id = link.second;
            //
            const auto itVData = valuedata.find(code);
            if (itVData == valuedata.end()) {
                continue; // 価格データがまだない
            }
            //
            const auto itEmStat
                = std::find_if(m_emergency_state.begin(),
                               m_emergency_state.end(),
                               [code, tactics_id](const EmergencyModeState& emstat)
            {
                return emstat.m_code == code && emstat.m_tactics_id == tactics_id;
            });
            //
            const auto& r_group =
                (itEmStat != m_emergency_state.end()) ?itEmStat->m_group 
                                                      :blank_group;
            auto tactics(m_tactics[tactics_id]);
            tactics.Interpret(investments, now_time, sec_time,
                              r_group, itVData->second,
                              script_mng,
                              [this, investments](const StockTradingCommandPtr& command_ptr)
            {
                EntryCommand(command_ptr, investments);
            });
        }
    }

    /*!
     *  @brief  任意の命令を処理する
     *  @param  command     命令
     *  @param  investments 取引所種別
     *  @param  aes_pwd
     *  @param  tickCount   経過時間[ミリ秒]
     */
    bool IssueOrderCore(const StockTradingCommand& command,
                        eStockInvestmentsType investments,
                        const garnet::CipherAES_string& aes_pwd,
                        int64_t tickCount)
    {
        const auto callback = [this, investments](bool b_result,
                                                  const RcvResponseStockOrder& rcv_order,
                                                  const std::wstring& sv_date) {
            StockOrderCallback(b_result, rcv_order, sv_date, investments);
        };

        if (!command.IsOrder()) {
            // 発注命令じゃない(error)
            return false;
        }
        if (m_b_lock_odmng_and_wait_execinfo) {
            // 次約定情報取得までロック
            return false;
        }
        const int64_t MIN_ORDER_INTV_TICK = garnet::utility_datetime::ToMiliSecondsFromSecond(1);
        if (tickCount < m_last_tick_rcv_rep_order + MIN_ORDER_INTV_TICK) {
            // 連射禁止
            return false;
        }

        const StockOrder order(std::move(command.GetOrder()));
        const StockCode& s_code(order.RefCode());

        std::wstring pwd;
        if (!aes_pwd.Decrypt(pwd)) {
            // 復号失敗
            return false;
        }

        switch (order.m_type)
        {
        case ORDER_BUY:
            m_pSecSession->BuySellOrder(order, pwd, callback);
            break;
        case ORDER_SELL:
            if (!m_holdings.CheckSpotTradingStock(s_code, order.GetNumber())) {
                // 株不足(ラグで起こり得る)
                return false;
            }
            m_pSecSession->BuySellOrder(order, pwd, callback);
            break;
        case ORDER_CORRECT:
            m_pSecSession->CorrectOrder(command.GetOrderID(), order, pwd, callback);
            break;
        case ORDER_CANCEL:
            m_pSecSession->CancelOrder(command.GetOrderID(), pwd, callback);
            break;
        case ORDER_REPSELL:
        case ORDER_REPBUY:
            {
                const garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                const float64 bg_value = command.GetRepLevBargainValue();
                const bool b_sell = order.m_type == ORDER_REPBUY; // 返買ならば売建玉を調べる
                if (!m_holdings.CheckPosition(s_code,
                                              bg_date,
                                              bg_value,
                                              b_sell,
                                              order.GetNumber())) {
                    // 株不足(ラグで起こり得る)
                    return false;
                }
                m_pSecSession->RepaymentLeverageOrder(bg_date,
                                                      bg_value,
                                                      order,
                                                      pwd,
                                                      callback);
            }
            break;
        default:
            // 不正な命令が積まれてる(error)
            return false;
        }
        //
        return true;
    }

    /*!
     *  @brief  命令リスト先頭の命令を処理する
     *  @param  investments 取引所種別
     *  @param  aes_pwd
     *  @param  tickCount   経過時間[ミリ秒]
     */
    void IssueOrder(eStockInvestmentsType investments,
                    const garnet::CipherAES_string& aes_pwd,
                    int64_t tickCount)
    {
        if (m_command_list.empty()) {
            return; // 空
        }
        if (!m_wait_order.empty()) {
            return; // 待ちがある
        }
        const StockTradingCommandPtr& command_ptr(m_command_list.front());
        const StockTradingCommand& command(*command_ptr);

        m_wait_order.push_back(command_ptr);
        m_command_list.pop_front();

        if (!IssueOrderCore(command, investments, aes_pwd, tickCount)) {
            // 発注できなかったら結果待ち削除(ラグ等で起こり得る)
            m_wait_order.pop_back();
        }
    }


public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const SecuritiesSessionPtr& sec_session,
          const garnet::TwitterSessionForAuthorPtr& tw_session,
          TradeAssistantSetting& script_mng)
    : m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_tactics()
    , m_tactics_link()
    , m_emergency_time_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(script_mng.GetEmergencyCoolSecond()))
    , m_monitoring_brand()
    , m_monitoring_data()
    , m_holdings()
    , m_b_lock_odmng_and_wait_execinfo(false)
    , m_command_list()
    , m_emergency_state()
    , m_wait_order()
    , m_server_order()
    , m_exec_order()
    , m_server_order_id()
    , m_investments(INVESTMENTS_NONE)
    , m_tick_count(0)
    , m_last_tick_rcv_rep_order(0)
    {
        UpdateMessage msg;
        if (!script_mng.BuildStockTactics(msg, m_tactics, m_tactics_link)) {
            // 失敗(error)
        }
    }

    /*!
     *  @brief  証券会社からの返答を待ってるか
     *  @retval true    発注結果待ちしてる
     */
    bool IsInWaitMessageFromSecurities() const
    {
        return !m_wait_order.empty();
    }
    
    /*!
     *  @brief  監視銘柄コード取得
     *  @param[out] dst 格納先
     */
    void GetMonitoringCode(StockCodeContainer& dst)
    {
        for (const auto& link: m_tactics_link) {
            dst.insert(link.first);
        }
    }

    /*!
     *  @brief  監視銘柄初期化
     *  @param  investments_type    取引所種別
     *  @param  rcv_brand_data      受信した監視銘柄群
     *  @retval true                成功
     */
    bool InitMonitoringBrand(eStockInvestmentsType investments_type,
                             const StockBrandContainer& rcv_brand_data)
    {
        // 受信した監視銘柄群が戦略データと一致してるかチェック
        std::unordered_map<uint32_t, StockValueData> monitoring_data;
        StockCodeContainer monitoring_code;
        GetMonitoringCode(monitoring_code);
        monitoring_data.reserve(monitoring_code.size());
        for (uint32_t code: monitoring_code) {
            if (rcv_brand_data.end() != rcv_brand_data.find(code)) {
                monitoring_data.emplace(code, StockValueData(code));
            } else {
                return false;
            }
        }
        // 空だったら新規作成
        auto itMtd = m_monitoring_data.find(investments_type);
        if (itMtd == m_monitoring_data.end()) {
            m_monitoring_data.emplace(investments_type, monitoring_data);
            m_monitoring_brand = rcv_brand_data;
        }
        return true;
    }

    /*!
     *  @brief  監視銘柄情報出力
     *  @param  log_dir 出力ディレクトリ
     *  @param  date    年月日
     */
    void OutputMonitoringLog(const std::string& log_dir, const garnet::YYMMDD& date) const
    {
        const auto outputLog = [log_dir, date](StockValueData vdata, std::string pts_tag)
        {
            const std::string code_str(std::move(std::to_string(vdata.m_code.GetCode())));
            const std::string date_str(std::move(date.to_string()));
            vdata.OutputLog(std::move(log_dir + pts_tag + code_str + "_" + date_str + ".csv"));
        };
        const auto itPTS = m_monitoring_data.find(INVESTMENTS_PTS);
        if (itPTS != m_monitoring_data.end()) {
            for (const auto& md: itPTS->second) {
                std::thread t(outputLog, md.second, "pts_");
                t.detach();
            }
        }
        const auto itTKY = m_monitoring_data.find(INVESTMENTS_TOKYO);
        if (itTKY != m_monitoring_data.end()) {
            for (const auto& md: itTKY->second) {
                std::thread t(outputLog, md.second, std::string());
                t.detach();
            }
        }
    }
    
    /*!
     *  @brief  価格データ更新
     *  @param  investments_type    取引所種別
     *  @param  senddate            価格データ送信時刻
     *  @param  rcv_valuedata       受け取った価格データ
     */
    void UpdateValueData(eStockInvestmentsType investments_type,
                         const std::wstring& sendtime,
                         const std::vector<RcvStockValueData>& rcv_valuedata)
    {
        auto itMtd = m_monitoring_data.find(investments_type);
        if (itMtd != m_monitoring_data.end()) {
            garnet::sTime tm_send; // 価格データ送信時刻(サーバタイム)
            auto pt(garnet::utility_datetime::ToLocalTimeFromRFC1123(sendtime));
            garnet::utility_datetime::ToTimeFromBoostPosixTime(pt, tm_send);
            auto& valuedata(itMtd->second);
            for (const auto& vunit: rcv_valuedata) {
                auto it = valuedata.find(vunit.m_code);
                if (it != valuedata.end()) {
                    it->second.UpdateValueData(vunit, tm_send);
                } else {
                    // 見つからなかったらどうする？(error)
                }
            }
        } else {
            // なぜか初期化されてない(error)
        }
    }

    /*!
     *  @brief  発注済み注文検索
     *  @param  src             発注済み注文map(1取引所分)
     *  @param  user_order_id   探す注文番号(表示用)
     *  @return iterator
     */
    std::unordered_map<int32_t, StockTradingCommandPtr>::iterator
        SearchServerOrder(std::unordered_map<int32_t, StockTradingCommandPtr>& src,
                          int32_t user_order_id)
    {
        const auto itID = m_server_order_id.find(user_order_id);
        if (itID == m_server_order_id.end()) {
            return src.end(); // ID変換失敗(error)
        }
        int32_t order_id = itID->second;
        return src.find(order_id);
    }

    /*!
     *  @brief  当日約定情報更新
     *  @param  rcv_info    受け取った約定情報
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info)
    {
        m_b_lock_odmng_and_wait_execinfo = false;

        // 約定差分(前回更新後に約定した情報)を得る
        std::vector<StockExecInfoAtOrder> diff_info;
        m_holdings.GetExecInfoDiff(rcv_info, diff_info);
        if (diff_info.empty()) {
            return ; // 変化なし
        }
        // 約定差分と対応する「発注済み信用返済売買注文」を取り出す
        ServerRepLevOrder rep_order;
        for (const auto& ex_info: diff_info) {
            if (ex_info.m_type != ORDER_REPBUY && ex_info.m_type != ORDER_REPSELL) {
                continue; // 対象は信用返済売買のみ
            }
            auto& sv_order(m_server_order[INVESTMENTS_TOKYO]);
            const int32_t user_order_id = ex_info.m_user_order_id;
            const auto itOrder = SearchServerOrder(sv_order, user_order_id);
            if (itOrder == sv_order.end()) {
                // 発注済み注文が無い(error) ... (a)
                // ツール外でなんらか発注してた場合はあり得る
                continue;
            }
            rep_order.emplace(user_order_id, itOrder->second);
        }
        // 保有銘柄管理更新
        m_holdings.UpdateExecInfo(rcv_info, diff_info, rep_order);
        // 約定済み注文更新
        for (auto it = m_exec_order.begin(); it != m_exec_order.end(); it++) {
            // 紐付いた"保有建玉"がなくなってたら"約定済み注文"も削除
            const StockCode s_code(it->first);
            auto itRmv = std::remove_if(it->second.begin(),
                                        it->second.end(),
                                        [this, &s_code](const StockExecOrderIdentifier& ex) {
                return !m_holdings.CheckPosition(s_code, ex.second);
            });
            if (it->second.end() != itRmv) {
                it->second.erase(itRmv, it->second.end());
            }
        }
        // 発注済み注文更新
        for (const auto& ex_info: diff_info) {
            auto& sv_order(m_server_order[ex_info.m_investments]);
            const int32_t user_order_id = ex_info.m_user_order_id;
            const auto itOrder = SearchServerOrder(sv_order, user_order_id);
            if (itOrder == sv_order.end()) {
                continue; // (a)に同じ
            }
            // 約定通知
            OutputExecMessage(itOrder->second, ex_info);
            // 全部約定
            if (ex_info.m_b_complete) {
                // 現物買/新規信用売買ならば「約定済み注文」へ登録
                const uint32_t code = ex_info.m_code;
                const eOrderType type = ex_info.m_type;
                if (ex_info.m_b_leverage && (type == ORDER_BUY || type == ORDER_SELL)) {
                    std::vector<int32_t> pos_id;
                    m_holdings.GetPositionID(user_order_id, pos_id);
                    m_exec_order[code].emplace_back(*itOrder->second, pos_id);
                } else if (type == ORDER_BUY) {
                    m_exec_order[code].emplace_back(*itOrder->second, std::vector<int32_t>());
                }
                // "発注済み注文"から削除
                sv_order.erase(itOrder);
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
        m_holdings.UpdateHoldings(spot, position);
    }

    /*!
     *  @brief  Update関数
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  now_time    現在時分秒
     *  @param  sec_time    現セクション開始時刻
     *  @param  investments 取引所種別
     *  @param  valuedata   価格データ(1取引所分)
     *  @param  aes_pwd
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void Update(int64_t tickCount,
                const garnet::HHMMSS& now_time,
                const garnet::HHMMSS& sec_time,
                eStockInvestmentsType investments,
                const garnet::CipherAES_string& aes_pwd,
                TradeAssistantSetting& script_mng)
    {
        // 取引所種別が変わったら今ある命令リストetcを破棄
        if (investments != m_investments) {
            m_command_list.clear();
            m_emergency_state.clear();
        }

        // 緊急モード状態更新
        {
            const int64_t diff_time = tickCount - m_tick_count;
            auto itRmv = std::remove_if(m_emergency_state.begin(),
                                        m_emergency_state.end(),
                                        [diff_time](EmergencyModeState& emstat)
            {
                emstat.m_timer -= diff_time;
                return emstat.m_timer <= 0;
            });
            if (itRmv != m_emergency_state.end()) {
                m_emergency_state.erase(itRmv, m_emergency_state.end());
            }
        }
        // 戦略解釈
        InterpretTactics(investments, now_time, sec_time,
                         m_monitoring_data[investments], script_mng);
        // 命令処理
        IssueOrder(investments, aes_pwd, tickCount);
        //
        m_tick_count = tickCount;
        m_investments = investments;
    }

};

/*!
 *  @param  sec_session 証券会社とのセッション
 *  @param  tw_session  twitterとのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockOrderingManager::StockOrderingManager(const SecuritiesSessionPtr& sec_session,
                                           const garnet::TwitterSessionForAuthorPtr& tw_session,
                                           TradeAssistantSetting& script_mng)
: m_pImpl(new PIMPL(sec_session, tw_session, script_mng))
{
}
/*!
 */
StockOrderingManager::~StockOrderingManager()
{
}

/*!
 *  @brief  証券会社からの返答を待ってるか
 *  @note   発注してる最中ならばtrue
 */
bool StockOrderingManager::IsInWaitMessageFromSecurities() const
{
    return m_pImpl->IsInWaitMessageFromSecurities();
}

/*!
 *  @brief  監視銘柄コード取得
 *  @param[out] dst 格納先
 */
void StockOrderingManager::GetMonitoringCode(StockCodeContainer& dst)
{
    m_pImpl->GetMonitoringCode(dst);
}
/*!
 *  @brief  監視銘柄初期化
 *  @param  investments_type    取引所種別
 *  @param  rcv_brand_data      受信した監視銘柄群
 */
bool StockOrderingManager::InitMonitoringBrand(eStockInvestmentsType investments_type,
                                               const StockBrandContainer& rcv_brand_data)
{
    return m_pImpl->InitMonitoringBrand(investments_type, rcv_brand_data);
}
/*!
 *  @brief  監視銘柄情報出力
 *  @param  log_dir 出力ディレクトリ
 *  @param  date    年月日
 */
void StockOrderingManager::OutputMonitoringLog(const std::string& log_dir,
                                               const garnet::YYMMDD& date)
{
    m_pImpl->OutputMonitoringLog(log_dir, date);
}

/*!
 *  @brief  保有銘柄更新
 *  @param  spot        現物保有株
 *  @param  position    信用保有株
 */
void StockOrderingManager::UpdateHoldings(const SpotTradingsStockContainer& spot,
                                            const StockPositionContainer& position)
{
    m_pImpl->UpdateHoldings(spot, position);
}

/*!
 *  @brief  価格データ更新
 *  @param  investments_type    取引所種別
 *  @param  senddate            価格データ送信時刻
 *  @param  rcv_valuedata       受け取った価格データ
 */
void StockOrderingManager::UpdateValueData(eStockInvestmentsType investments_type,
                                           const std::wstring& sendtime,
                                           const std::vector<RcvStockValueData>& rcv_valuedata)
{
    m_pImpl->UpdateValueData(investments_type, sendtime, rcv_valuedata);
}

/*!
 *  @brief  当日約定情報更新
 *  @param  rcv_info    受け取った約定情報
 */
void StockOrderingManager::UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info)
{
    m_pImpl->UpdateExecInfo(rcv_info);
}

/*!
 *  @brief  Update関数
 *  @param  tickCount   経過時間[ミリ秒]
 *  @param  now_time    現在時分秒
 *  @param  sec_time    現セクション開始時刻
 *  @param  investments 取引所種別
 *  @param  valuedata   価格データ(1取引所分)
 *  @param  aes_pwd
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
void StockOrderingManager::Update(int64_t tickCount,
                                  const garnet::HHMMSS& now_time,
                                  const garnet::HHMMSS& sec_time,
                                  eStockInvestmentsType investments,
                                  const garnet::CipherAES_string& aes_pwd,
                                  TradeAssistantSetting& script_mng)
{
    m_pImpl->Update(tickCount, now_time, sec_time, investments, aes_pwd, script_mng);
}

} // namespace trading
