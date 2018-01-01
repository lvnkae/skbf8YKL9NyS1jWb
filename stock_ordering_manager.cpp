/*!
 *  @file   stock_ordering_manager.cpp
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#include "stock_ordering_manager.h"

#include "securities_session.h"
#include "stock_trading_command.h"
#include "stock_trading_tactics.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"

#include "cipher_aes.h"
#include "twitter_session.h"
#include "utility_datetime.h"
#include "utility_string.h"

#include <list>
#include <stack>
#include <unordered_map>

namespace trading
{
class StockOrderingManager::PIMPL
{
private:
    /*!
     *  @brief  注文識別情報
     */
    struct StockOrderIdentifier
    {
        int32_t m_tactics_id;   //!< 戦略ID
        int32_t m_group_id;     //!< 戦略内グループID
        std::wstring m_name;    //!< 名前(主に銘柄名)

        StockOrderIdentifier(int32_t t, int32_t g, const std::wstring& name)
        : m_tactics_id(t)
        , m_group_id(g)
        , m_name(name)
        {
        }
    };

    //! 証券会社とのセッション
    std::shared_ptr<SecuritiesSession> m_pSecSession;
    //! twitterとのセッション
    std::shared_ptr<TwitterSessionForAuthor> m_pTwSession;
    //! 命令リスト<所属戦略id, 命令>
    std::list<std::pair<int32_t, StockTradingCommand>> m_command_list;
    //! 緊急モード対象者<戦略id, 残期間(ミリ秒)>
    std::unordered_map<int32_t, int64_t> m_emergency_tactics;
    //! 緊急モード期間[ミリ秒] ※外部設定から取得
    const int64_t m_emergency_time_ms;
    //! 注文結果待ちOrder
    std::stack<std::pair<StockOrderIdentifier, StockOrder>> m_wait_order;
    //! 発注状況<order_id, order>
    std::unordered_map<int32_t, std::pair<StockOrderIdentifier, StockOrder>> m_server_order;
    //! 現在の対象取引所 ※Interpret終了時に更新
    eStockInvestmentsType m_investments;
    //! 経過時間[ミリ秒] ※Interpret終了時に更新
    int64_t m_tick_count;

    /*!
     *  @brief  売買注文結果メッセージ出力
     *  @param  b_result    成否
     *  @param  order       注文パラメータ
     *  @param  order_id    注文番号
     *  @param  name        銘柄名
     *  @param  err         エラーメッセージ
     *  @param  date        発生日時
     *  @note   出力先はtwitter
     */
    void OutputMessage(bool b_result,
                       const StockOrder& order,
                       int32_t order_id,
                       const std::wstring& name,
                       const std::wstring& err,
                       const std::wstring& date)
    {
        const std::wstring nl(std::move(twitter::GetNewlineString()));
        std::wstring src((b_result) ?L"注文受付" : L"注文失敗");
        if (order.m_type == ORDER_NONE) {
            // orderが空で呼ばれることもある(発注失敗時)
        } else {
            if (order.m_type == ORDER_BUY) {
                if (order.m_b_leverage) {
                    src += L"(信用新規買)";
                } else {
                    src += L"(現物買)";
                }
            } else if (order.m_type == ORDER_SELL) {
                src += L"(信用新規売)";
            }
            src += L" " + utility::ToSecuretIDOrder(order_id, 4);
            src += nl + std::to_wstring(order.m_code.GetCode()) + L" " + name;
            src += nl + L"株数 " + std::to_wstring(order.m_number);
            src += nl + L"価格 " + utility::ToWstringOrder(order.m_value, 1);
            if (order.m_b_market_order) {
                src += L"(成行)";
            }
        }
        if (!err.empty()) {
            src += nl + L"[error] " + err;
        }
        m_pTwSession->Tweet(date, src);
    }

    /*!
     *  @brief  売買注文コールバック
     *  @param  b_result    成否
     *  @param  rcv_order   注文結果
     *  @param  sv_date     サーバ時刻
     */
    void StockOrderCallback(bool b_result, const RcvResponseStockOrder& rcv_order, const std::wstring& sv_date)
    { 
        if (m_wait_order.empty()) {
            // なぜか注文待ちがない(error)
            const std::wstring err_msg(L"$wait_order is empty");
            std::wstring stk_name;
            OutputMessage(b_result, StockOrder(rcv_order), rcv_order.m_order_id, stk_name, err_msg, sv_date);
        } else {
            const auto& w_order = m_wait_order.top();
            const StockOrderIdentifier& soidx = w_order.first;
            const StockOrder& order = w_order.second;
            const std::wstring& stk_name = w_order.first.m_name;
            //
            std::wstring err_msg;
            if (b_result) {
                const float64 diff_value = order.m_value - rcv_order.m_value;
                if (order.m_code.GetCode() == rcv_order.m_code &&
                    order.m_type == rcv_order.m_type &&
                    order.m_investiments == rcv_order.m_investments &&
                    ((order.m_b_leverage && rcv_order.m_b_leverage) || (!order.m_b_leverage && !rcv_order.m_b_leverage)) &&
                    (-0.05 < diff_value && diff_value < 0.05) && /* 誤差0.05未満は許容(浮動小数点の一致比較はあかんので) */
                    order.m_number == rcv_order.m_number) {
                } else {
                    // 受付と待ちが食い違ってる(error)
                    err_msg = L"isn't equal $wait_order and $rcv_order";
                }
                m_server_order.emplace(rcv_order.m_order_id, std::pair<StockOrderIdentifier, StockOrder>(soidx, order));
            }
            // 通知(m_wait_orderを参照してるので↑↓で処理が別れてる)
            OutputMessage(b_result, order, rcv_order.m_order_id, stk_name, err_msg, sv_date);
            // 受け付けられたら注文待ち解除
            if (b_result) {
                m_wait_order.pop();
            }
        }
    }

public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const std::shared_ptr<SecuritiesSession>& sec_session,
          const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
          const TradeAssistantSetting& script_mng)
    : m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_command_list()
    , m_emergency_tactics()
    , m_emergency_time_ms(utility::ToMiliSecondsFromSecond(script_mng.GetEmergencyCoolSecond()))
    , m_investments(INVESTMENTS_NONE)
    , m_tick_count(0)
    {
    }

    /*!
     *  @brief  machineパラメータ反映
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  investments 対象取引所
     */
    void Correct(int64_t tickCount, eStockInvestmentsType investments)
    {
        m_tick_count = tickCount;
        m_investments = investments;
    }

    /*!
     *  @brief  定期処理：緊急モード
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  tactics     取引戦略
     *  @retval true    当該戦略は緊急モード
     */
    bool Update_Emergency(int64_t tickCount, const StockTradingTactics& tactics)
    {
        auto it = m_emergency_tactics.find(tactics.GetUniqueID());
        if (it != m_emergency_tactics.end()) {
            // 指定期間が過ぎたら除外
            it->second -= (tickCount - m_tick_count);
            if (it->second > 0) {
                return true;
            } else {
                m_emergency_tactics.erase(it);
            }
        }

        return false;
    }

    /*!
     *  @brief  取引命令登録
     *  @param  tactics_id  戦略固有ID
     *  @param  command     命令
     */
    void EntryCommand(int32_t tactics_id, const StockTradingCommand& command)
    {
        const uint32_t code = command.GetCode();

        // 命令種別ごとの処理
        switch (command.GetType())
        {
        case StockTradingCommand::EMERGENCY:
            // 対象戦略IDに属する命令は捨てる
            std::remove_if(m_command_list.begin(),
                           m_command_list.end(),
                           [tactics_id, code](std::pair<int32_t, StockTradingCommand>& elem) {
                return (tactics_id == elem.first &&
                        code == elem.second.GetCode() &&
                        !elem.second.IsForEmergencyCommand()); // 緊急時命令は除外
            });
            // 禁止モード対象者とする
            m_emergency_tactics[tactics_id] = m_emergency_time_ms;
            // 先頭に積む
            m_command_list.emplace_front(tactics_id, command);
            break;

        case StockTradingCommand::ORDER:
            // 同一戦略/同一グループのORDERがあれば消す
            // 同発注されてしまってたら優先度にしたがって弾くか取り消すか決める(>ToDo<)
            for (const auto& sv_order: m_server_order) {
                if (sv_order.second.first.m_tactics_id == tactics_id &&
                    sv_order.second.first.m_group_id == command.GetGroupID()) {
                    return; // 弾く
                }
            }
            // 発注結果待ちしてても弾く(か取り消すか決める) >ToDo<
            if (!m_wait_order.empty()) {
                const auto& w_order = m_wait_order.top();
                if (w_order.first.m_tactics_id == tactics_id &&
                    w_order.first.m_group_id == command.GetGroupID()) {
                    return; // 弾く
                }
            }
            for (const auto& lcommand: m_command_list) {
                if (lcommand.second.GetType() == StockTradingCommand::ORDER) {
                    if (lcommand.second.GetGroupID() == command.GetGroupID() &&
                        lcommand.second.GetTacticsID() == command.GetTacticsID()) {
                        return; // 弾く
                    }
                }
            }
            m_command_list.emplace_back(tactics_id, command);
            break;

        default:
            // 末尾に積む
            m_command_list.emplace_back(tactics_id, command);
            break;
        }
    }

    /*!
     *  @brief  命令を処理する
     *  @param  aes_pwd
     */
    void IssueOrder(const CipherAES& aes_pwd)
    {
        if (m_command_list.empty()) {
            return; // 空
        }
        if (!m_wait_order.empty()) {
            return; // 待ちがある
        }
        auto it = m_command_list.begin();
        const StockTradingCommand& command(it->second);
        const StockOrderIdentifier soidx(command.GetTacticsID(), command.GetGroupID(), command.GetName());
        StockOrder order;
        order.m_investiments = m_investments;
        if (command.GetType() == StockTradingCommand::ORDER) {
            command.GetOrder(order);
            if (order.m_type == ORDER_BUY || order.m_type == ORDER_SELL) {
                m_wait_order.push(std::pair<StockOrderIdentifier, StockOrder>(soidx, order));
                std::wstring pwd;
                aes_pwd.Decrypt(pwd);
                m_pSecSession->FreshOrder(order, pwd,
                                          [this](bool b_result,
                                                 const RcvResponseStockOrder& rcv_order,
                                                 const std::wstring& sv_date)
                {
                    StockOrderCallback(b_result, rcv_order, sv_date);
                });
            }
        }
        m_command_list.pop_front();
    }
};

/*!
 *  @param  sec_session 証券会社とのセッション
 *  @param  tw_session  twitterとのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockOrderingManager::StockOrderingManager(const std::shared_ptr<SecuritiesSession>& sec_session,
                                           const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
                                           const TradeAssistantSetting& script_mng)
: m_pImpl(new PIMPL(sec_session, tw_session, script_mng))
{
}
/*!
 */
StockOrderingManager::~StockOrderingManager()
{
}

/*!
 *  @brief  取引戦略解釈
 *  @param  tickCount   経過時間[ミリ秒]
 *  @param  now_tm      現在時分秒
 *  @param  tactics     戦略データ
 *  @param  valuedata   価格データ(1取引所分)
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
void StockOrderingManager::InterpretTactics(int64_t tickCount,
                                            const HHMMSS& hhmmss,
                                            eStockInvestmentsType investments,
                                            const std::vector<StockTradingTactics>& tactics,
                                            const std::vector<StockPortfolio>& valuedata,
                                            TradeAssistantSetting& script_mng)
{
    // >ToDo< 取引所種別が変わったら今ある命令リストetcを破棄

    // 解釈
    for (const auto& tac: tactics) {
        const bool b_emergency = m_pImpl->Update_Emergency(tickCount, tac);

        tac.Interpret(b_emergency,
                      hhmmss,
                      valuedata,
                      script_mng,
                      [this](int32_t tactics_id, const StockTradingCommand& command) {
            m_pImpl->EntryCommand(tactics_id, command);
        });
    }

    m_pImpl->Correct(tickCount, investments);
}

/*!
 *  @brief  命令を処理する
 *  @param  aes_pwd
 */
void StockOrderingManager::IssueOrder(const CipherAES& aes_pwd)
{
    m_pImpl->IssueOrder(aes_pwd);
}

} // namespace trading
