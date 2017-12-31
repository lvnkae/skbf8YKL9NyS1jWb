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
#include "utility_datetime.h"

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

        StockOrderIdentifier(int32_t t, int32_t g)
        : m_tactics_id(t)
        , m_group_id(g)
        {
        }
    };

    //! 証券会社とのセッション
    std::shared_ptr<SecuritiesSession> m_pSession;
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
    //! 現在の対象取引所
    eStockInvestmentsType m_investments;
    //! 経過時間[ミリ秒] ※Interpret終了時に更新
    int64_t m_tick_count;


    /*!
     *  @brief  売買注文コールバック
     *  @param  b_result    成否
     *  @param  rcv_order   注文結果
     */
    void StockOrderCallback(bool b_result, const RcvResponseStockOrder& rcv_order)
    { 
        if (m_wait_order.empty()) {
            return; // 致命的なエラー(error)
        }
        const auto& w_order = m_wait_order.top();
        const StockOrderIdentifier& soidx = w_order.first;
        const StockOrder& order = w_order.second;
        if (b_result) {
            const float64 diff_value = order.m_value - rcv_order.m_value;
            if (order.m_code.GetCode() == rcv_order.m_code &&
                order.m_type == rcv_order.m_type &&
                order.m_investiments == rcv_order.m_investments &&
                ((order.m_b_leverage && rcv_order.m_b_leverage) || (!order.m_b_leverage && !rcv_order.m_b_leverage)) &&
                (-0.05 < diff_value && diff_value < 0.05) && /* 誤差0.05未満は許容(浮動小数点の一致比較はあかんので) */
                order.m_number == rcv_order.m_number)
            {
                m_server_order.emplace(rcv_order.m_order_id, std::pair<StockOrderIdentifier, StockOrder>(soidx, order));
                // >ToDo< 注文成功したよ(twitter通知)
            }
        } else {
            // >ToDo< 注文失敗したよ(twitter通知)
        }
        m_wait_order.pop();
    }

public:
    /*!
     *  @param  session     証券会社とのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const std::shared_ptr<SecuritiesSession>& session, const TradeAssistantSetting& script_mng)
    : m_pSession(session)
    , m_command_list()
    , m_emergency_tactics()
    , m_emergency_time_ms(utility::ToMiliSecondsFromSecond(script_mng.GetEmergencyCoolSecond()))
    , m_investments(INVESTMENTS_NONE)
    , m_tick_count(0)
    {
    }

    /*!
     *  @brief  machineパラメータ更新
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
        const StockOrderIdentifier soidx(command.GetTacticsID(), command.GetGroupID());
        StockOrder order;
        order.m_investiments = m_investments;
        if (command.GetType() == StockTradingCommand::ORDER) {
            command.GetOrder(order);
            if (order.m_type == ORDER_BUY || order.m_type == ORDER_SELL) {
                m_wait_order.push(std::pair<StockOrderIdentifier, StockOrder>(soidx, order));
                std::wstring pwd;
                aes_pwd.Decrypt(pwd);
                m_pSession->FreshOrder(order, pwd, [this](bool b_result, const RcvResponseStockOrder& rcv_order) {
                    StockOrderCallback(b_result, rcv_order);
                });
            }
        }
        m_command_list.pop_front();
    }
};

/*!
 *  @param  session     証券会社とのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockOrderingManager::StockOrderingManager(const std::shared_ptr<SecuritiesSession>& session,
                                           const TradeAssistantSetting& script_mng)
: m_pImpl(new PIMPL(session, script_mng))
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
