/*!
 *  @file   stock_ordering_manager.cpp
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#include "stock_ordering_manager.h"

#include "securities_session.h"
#include "stock_portfolio.h"
#include "stock_trading_command.h"
#include "stock_trading_tactics.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"

#include "cipher_aes.h"
#include "twitter_session.h"
#include "update_message.h"
#include "utility_datetime.h"
#include "utility_string.h"

#include <list>

namespace trading
{
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

        EmergencyModeState(uint32_t code, int32_t tactics_id, const std::vector<int32_t>& group, int64_t timer)
        : m_code(code)
        , m_tactics_id(tactics_id)
        , m_group()
        , m_timer(timer)
        {
            AddGroupID(group);
        }

        void AddGroupID(const std::vector<int32_t>& group)
        {
            for (int32_t group_id: group) {
                m_group.insert(group_id);
            }
        }
    };

    //! 証券会社とのセッション
    std::shared_ptr<SecuritiesSession> m_pSecSession;
    //! twitterとのセッション
    std::shared_ptr<TwitterSessionForAuthor> m_pTwSession;

    //! 取引戦略データ<戦略ID, 戦略データ>
    std::unordered_map<int32_t, StockTradingTactics> m_tactics;
    //! 戦略データ紐付け情報<銘柄コード, 戦略ID>
    std::vector<std::pair<uint32_t, int32_t>> m_tactics_link;
    //! 緊急モード期間[ミリ秒] ※外部設定から取得
    const int64_t m_emergency_time_ms;
    //! 銘柄名<銘柄コード, 名前(utf-16)>
    std::unordered_map<uint32_t, std::wstring> m_stock_name;
    //! 監視銘柄データ<取引所種別, <銘柄コード, 1銘柄分の監視銘柄データ>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<uint32_t, StockPortfolio>> m_portfolio;

    //! 命令リスト
    std::list<StockTradingCommand> m_command_list;
    //! 緊急モード状態
    std::list<EmergencyModeState> m_emergency_state;
    //! 結果待ち注文<識別情報, 注文パラメータ> ※要素数は1か0/注文は1つずつ処理する
    std::vector<std::pair<TacticsIdentifier, StockOrder>> m_wait_order;
    //! 発注状況<取引所種別, <管理用注文番号, <識別情報, 注文パラメータ>>>
    std::unordered_map<eStockInvestmentsType, std::unordered_map<int32_t, std::pair<TacticsIdentifier, StockOrder>>> m_server_order;
    //! 注文番号対応表<表示用注文番号, 管理用注文番号>
    std::unordered_map<int32_t, int32_t> m_server_order_id;

    //! 現在の対象取引所
    eStockInvestmentsType m_investments;
    //! 経過時間[ミリ秒]
    int64_t m_tick_count;

    /*!
     *  @brief  命令と注文の属性比較
     *  @param  left    命令
     *  @param  right   注文+識別情報
     */
    bool static CompareOrderAttr(const StockTradingCommand& left, const std::pair<TacticsIdentifier, StockOrder>& right)
    {
        // 同種・同戦略の注文ならばtrue
        return left.GetType() == StockTradingCommand::ORDER &&
               left.GetOrderType() == static_cast<int>(right.second.m_type) &&
               left.GetCode() == right.second.m_code.GetCode() &&
               left.GetTacticsID() == right.first.m_tactics_id &&
               left.GetOrderGroupID() == right.first.m_group_id;
    }

    void AddErrorMsg(const std::wstring& err, std::wstring& dst)
    {
        if (dst.empty()) {
            dst = err;
        } else {
            dst += twitter::GetNewlineString() + err;
        }
    }

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
                       const std::wstring& err,
                       const std::wstring& date)
    {
        const std::wstring nl(std::move(twitter::GetNewlineString()));
        std::wstring src((b_result) ?L"注文受付" : L"注文失敗");
        if (order.m_type == ORDER_NONE) {
            // orderが空で呼ばれることもある(発注失敗時)
        } else {
            const uint32_t code = order.m_code.GetCode();
            switch (order.m_type)
            {
            case ORDER_BUY:
                if (order.m_b_leverage) {
                    src += L"(信用新規買)";
                } else {
                    src += L"(現物買)";
                }
                break;
            case ORDER_SELL:
                src += L"(信用新規売)";
                break;
            case ORDER_CORRECT:
                src += L"(注文訂正)";
                break;
            case ORDER_CANCEL:
                src += L"(注文取消)";
                break;
            }
            src += L" " + utility::ToSecuretIDOrder(order_id, 4);
            src += nl + std::to_wstring(code) + L" " + m_stock_name[code];
            src += nl + L"株数 " + std::to_wstring(order.m_number);
            src += nl + L"価格 " + utility::ToWstringOrder(order.m_value, 1);
            if (order.m_b_market_order) {
                src += L"(成行)";
            }
        }
        if (!err.empty()) {
            src += nl + L"[error]" + nl + err;
        }
        m_pTwSession->Tweet(date, src);
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
        if (m_wait_order.empty()) {
            // なぜか注文待ちがない(error)
            const std::wstring err_msg(L"%wait_order is empty");
            OutputMessage(b_result, StockOrder(rcv_order), rcv_order.m_order_id, err_msg, sv_date);
        } else {
            const auto& w_order = m_wait_order.front();
            const TacticsIdentifier& tc_idx = w_order.first;
            const StockOrder& order = w_order.second;
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
                    err_msg = L"isn't equal %wait_order and %rcv_order";
                }
                switch (order.m_type)
                {
                case ORDER_BUY:
                case ORDER_SELL:
                    m_server_order[investments].emplace(rcv_order.m_order_id, std::pair<TacticsIdentifier, StockOrder>(tc_idx, order));
                    m_server_order_id.emplace(rcv_order.m_user_order_id, rcv_order.m_order_id);
                    break;
                case ORDER_CANCEL:
                    {
                        auto itID = m_server_order_id.find(rcv_order.m_user_order_id);
                        if (itID != m_server_order_id.end()) {
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
            // 通知(m_wait_orderを参照してるので↑↓で処理が別れてる)
            OutputMessage(b_result, order, rcv_order.m_user_order_id, err_msg, sv_date);
            // 受け付けられたら注文待ち解除
            if (b_result) {
                m_wait_order.pop_back();
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
        const std::vector<int32_t>& group = command.RefEmergencyTargetGroup();
        for (auto& emstat: m_emergency_state) {
            if (emstat.m_code == code && emstat.m_tactics_id == tactics_id) {
                // すでにあれば更新
                emstat.AddGroupID(group);
                emstat.m_timer = m_emergency_time_ms;
                return;
            }
        }
        m_emergency_state.emplace_back(code, tactics_id, group, m_emergency_time_ms);
    }

    /*!
     *  @brief  取引命令登録
     *  @param  command     命令
     */
    void EntryCommand(const StockTradingCommand& command)
    {
        const uint32_t code = command.GetCode();
        const int32_t tactics_id = command.GetTacticsID();

        // 命令種別ごとの処理
        switch (command.GetType())
        {
        case StockTradingCommand::EMERGENCY:
            // 緊急モード状態登録
            EntryEmergencyState(command);
            // 発注前注文削除
            {
                auto itRmv = std::remove_if(m_command_list.begin(),
                                            m_command_list.end(),
                                            [&command, this](StockTradingCommand& dst) {
                    if (dst.GetType() == StockTradingCommand::ORDER &&
                        command.GetCode() == dst.GetCode() &&
                        command.GetTacticsID() == dst.GetTacticsID()) {
                        const eOrderType od_type = static_cast<eOrderType>(dst.GetOrderType());
                        if (od_type == ORDER_BUY || od_type == ORDER_SELL || od_type == ORDER_CORRECT) {
                            const int32_t group_id = dst.GetOrderGroupID();
                            for (const int32_t em_gid: command.RefEmergencyTargetGroup()) {
                                if (group_id == em_gid) {
                                    return true;
                                }
                            }
                            return false;
                        }
                    }
                    return false;
                });
                if (m_command_list.end() != itRmv) {
                    m_command_list.erase(itRmv, m_command_list.end());
                }
            }
            // 発注済み注文取消
            for (const auto& sv_order: m_server_order[m_investments]) {
                const TacticsIdentifier& tc_idx(sv_order.second.first);
                const StockOrder& order(sv_order.second.second);
                if (tc_idx.m_tactics_id == tactics_id && order.m_code.GetCode() == code) {
                    const int32_t order_id = sv_order.first;
                    auto itCr = std::find_if(m_command_list.begin(), m_command_list.end(),
                                             [order_id](const StockTradingCommand& com) {
                        return com.GetOrderType() == ORDER_CANCEL && com.GetOrderID() == order_id;
                    });
                    if (itCr != m_command_list.end()) {
                        continue; // もう積んである
                    }
                    const auto& em_group(command.RefEmergencyTargetGroup());
                    auto itEm = std::find(em_group.begin(), em_group.end(), tc_idx.m_group_id);
                    if (itEm != em_group.end()) {
                        // 注文取消を先頭に積む
                        StockTradingCommand cancel_command(order,
                                                           tactics_id,
                                                           tc_idx.m_group_id,
                                                           tc_idx.m_unique_id,
                                                           static_cast<int32_t>(ORDER_CANCEL),
                                                           order_id);
                        m_command_list.emplace_front(cancel_command);
                    }
                }
            }
            break;

        case StockTradingCommand::ORDER:
            // 発注結果待ち注文チェック
            if (!m_wait_order.empty()) {
                const auto& w_order = m_wait_order.front();
                if (PIMPL::CompareOrderAttr(command, w_order)) {
                    // 同属性注文待ちしてるので弾く(発注完了後に対処する)
                    return;
                }
            }
            // 発注済み注文チェック
            for (const auto& sv_order: m_server_order[m_investments]) {
                if (PIMPL::CompareOrderAttr(command, sv_order.second)) {
                    if (sv_order.second.first.m_unique_id >= command.GetOrderUniqueID()) {
                        // 同一、または同属上位注文発注済みなので無視
                        return;
                    }
                    // 同属下位命令発注済みのため価格訂正を末尾に積む
                    StockTradingCommand correct_command(command, static_cast<int32_t>(ORDER_CORRECT), sv_order.first);
                    m_command_list.emplace_back(correct_command);
                    return;
                }
            }
            // 発注待機注文チェック
            for (auto& lcommand: m_command_list) {
                if (lcommand.IsUpperBuySellOrder(command)) {
                    // 上書き(後勝ち)
                    lcommand = command;
                    return;
                }
            }
            // 末尾に積む
            m_command_list.emplace_back(command);
            break;

        default:
            break;
        }
    }


    /*!
     *  @brief  戦略解釈
     *  @param  hhmmss      現在時分秒
     *  @param  valuedata   価格データ(1取引所分)
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void InterpretTactics(const HHMMSS& hhmmss,
                          const std::unordered_map<uint32_t, StockPortfolio>& valuedata,
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
            const auto itEmStat = std::find_if(m_emergency_state.begin(),
                                               m_emergency_state.end(),
                                               [code, tactics_id](const EmergencyModeState& emstat)
            {
                return emstat.m_code == code && emstat.m_tactics_id == tactics_id;
            });
            //
            const auto& r_group = (itEmStat != m_emergency_state.end()) ?itEmStat->m_group :blank_group;
            m_tactics[tactics_id].Interpret(hhmmss,
                                            r_group,
                                            itVData->second,
                                            script_mng,
                                            [this](const StockTradingCommand& command) { EntryCommand(command); });
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
        const StockTradingCommand& command(m_command_list.front());
        if (command.GetType() != StockTradingCommand::ORDER) {
            // 不正な命令が積まれてたら消してret(error)
            m_command_list.pop_front();
            return;
        }

        // 結果待ち注文に積む
        StockOrder order;
        command.GetOrder(order);
        const eStockInvestmentsType investments = m_investments;
        order.m_investiments = investments;
        m_wait_order.emplace_back(std::pair<TacticsIdentifier, StockOrder>(command, order));
        //
        std::wstring pwd;
        aes_pwd.Decrypt(pwd);
        const auto callabck = [this, investments](bool b_result,
                                                  const RcvResponseStockOrder& rcv_order,
                                                  const std::wstring& sv_date) {
            StockOrderCallback(b_result, rcv_order, sv_date, investments);
        };
        switch (order.m_type)
        {
        case ORDER_BUY:
        case ORDER_SELL:
            m_pSecSession->FreshOrder(order, pwd, callabck);
            break;
        case ORDER_CORRECT:
            m_pSecSession->CorrectOrder(command.GetOrderID(), order, pwd, callabck);
            break;
        case ORDER_CANCEL:
            m_pSecSession->CancelOrder(command.GetOrderID(), pwd, callabck);
            break;
        default:
            // 不正な命令が積まれてたら、結果待ちから消す(error)
            m_wait_order.pop_back();
            break;
        }
        //
        m_command_list.pop_front();
    }

public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const std::shared_ptr<SecuritiesSession>& sec_session,
          const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
          TradeAssistantSetting& script_mng)
    : m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_tactics()
    , m_tactics_link()
    , m_emergency_time_ms(utility::ToMiliSecondsFromSecond(script_mng.GetEmergencyCoolSecond()))
    , m_stock_name()
    , m_portfolio()
    , m_command_list()
    , m_emergency_state()
    , m_wait_order()
    , m_server_order()
    , m_server_order_id()
    , m_investments(INVESTMENTS_NONE)
    , m_tick_count(0)
    {
        UpdateMessage msg;
        if (!script_mng.BuildStockTactics(msg, m_tactics, m_tactics_link)) {
            // 失敗
        }
    }


    /*!
     *  @brief  監視銘柄コード取得
     *  @param[out] dst 格納先
     */
    void GetMonitoringCode(std::unordered_set<uint32_t>& dst)
    {
        for (const auto& link: m_tactics_link) {
            dst.insert(link.first);
        }
    }

    /*!
     *  @brief  ポートフォリオ初期化
     *  @param  investments_type    取引所種別
     *  @param  rcv_portfolio       受信したポートフォリオ(簡易)
     *  @retval true                成功
     */
    bool InitPortfolio(eStockInvestmentsType investments_type,
                       const std::unordered_map<uint32_t, std::wstring>& rcv_portfolio)
    {
        // 受信したポートフォリオが監視銘柄と一致してるかチェック
        std::unordered_map<uint32_t, StockPortfolio> portfolio;
        std::unordered_set<uint32_t> monitoring_code;
        GetMonitoringCode(monitoring_code);
        portfolio.reserve(monitoring_code.size());
        for (uint32_t code: monitoring_code) {
            if (rcv_portfolio.end() != rcv_portfolio.find(code)) {
                portfolio.emplace(code, StockPortfolio(code));
            } else {
                return false;
            }
        }
        // 空だったら新規作成
        auto portfolio_it = m_portfolio.find(investments_type);
        if (portfolio_it == m_portfolio.end()) {
            m_portfolio.emplace(investments_type, portfolio);
            m_stock_name = rcv_portfolio;
        }
        return true;
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
        auto itPf = m_portfolio.find(investments_type);
        if (itPf != m_portfolio.end()) {
            std::tm tm_send; // 価格データ送信時刻(サーバタイム)
            auto pt(utility::ToLocalTimeFromRFC1123(sendtime));
            utility::ToTimeFromBoostPosixTime(pt, tm_send);
            auto& portfolio(itPf->second);
            for (const auto& vunit: rcv_valuedata) {
                auto it = portfolio.find(vunit.m_code);
                if (it != portfolio.end()) {
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
     *  @brief  定期更新
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  hhmmss      現在時分秒
     *  @param  investments 取引所種別
     *  @param  valuedata   価格データ(1取引所分)
     *  @param  aes_pwd
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void Update(int64_t tickCount,
                const HHMMSS& hhmmss,
                eStockInvestmentsType investments,
                const CipherAES& aes_pwd,
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
        InterpretTactics(hhmmss, m_portfolio[investments], script_mng);
        // 命令処理
        IssueOrder(aes_pwd);
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
StockOrderingManager::StockOrderingManager(const std::shared_ptr<SecuritiesSession>& sec_session,
                                           const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
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
 *  @brief  監視銘柄コード取得
 *  @param[out] dst 格納先
 */
void StockOrderingManager::GetMonitoringCode(std::unordered_set<uint32_t>& dst)
{
    m_pImpl->GetMonitoringCode(dst);
}

/*!
 *  @brief  ポートフォリオ初期化
 *  @param  investments_type    取引所種別
 *  @param  rcv_portfolio       受信したポートフォリオ<銘柄コード番号, 銘柄名(utf-16)>
 */
bool StockOrderingManager::InitPortfolio(eStockInvestmentsType investments_type,
                                         const std::unordered_map<uint32_t, std::wstring>& rcv_portfolio)
{
    return m_pImpl->InitPortfolio(investments_type, rcv_portfolio);
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
 *  @brief  定期更新
 *  @param  tickCount   経過時間[ミリ秒]
 *  @param  hhmmss      現在時分秒
 *  @param  investments 取引所種別
 *  @param  valuedata   価格データ(1取引所分)
 *  @param  aes_pwd
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
void StockOrderingManager::Update(int64_t tickCount,
                                  const HHMMSS& hhmmss,
                                  eStockInvestmentsType investments,
                                  const CipherAES& aes_pwd,
                                  TradeAssistantSetting& script_mng)
{
    m_pImpl->Update(tickCount, hhmmss, investments, aes_pwd, script_mng);
}


} // namespace trading
