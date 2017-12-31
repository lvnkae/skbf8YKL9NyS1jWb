/*!
 *  @file   stock_trading_starter_sbi.cpp
 *  @brief  株取引スタート係：SBI用
 *  @date   2017/12/20
 */
#include "stock_trading_starter_sbi.h"

#include "securities_session_sbi.h"
#include "trade_assistant_setting.h"

#include "cipher_aes.h"
#include "random_generator.h"
#include "utility_datetime.h"

namespace trading
{

class StockTradingStarterSbi::PIMPL
{
private:
    enum eSequence
    {
        SEQ_NONE,   //!< 何もしてない
        SEQ_BUSY,   //!< 処理中
        SEQ_READY,  //!< 準備OK
    };

    const int64_t m_session_keep_ms;                    //!< セッションを維持できる時間(ms)
    std::shared_ptr<SecuritiesSession> m_pSession;      //!< セッション
    eSequence m_sequence;                               //!< シーケンス
    eStockInvestmentsType m_last_portfolior_investments;//!< 最後に作ったポートフォリオの対象取引所種別

    /*!
     *  @brief  ポートフォリオ作成
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_type    取引所種別
     *  @param  init_portfolio      ポートフォリオ初期化関数
     */
    void CreatePortfolio(const std::vector<uint32_t>& monitoring_code,
                         eStockInvestmentsType investments_type,
                         const InitPortfolioFunc& init_portfolio)
    {
        if (m_last_portfolior_investments != investments_type) {
            m_pSession->CreatePortfolio(monitoring_code,
                                        investments_type,
                                        [this, 
                                         investments_type,
                                         init_portfolio](bool b_result,
                                                         const std::vector<std::pair<uint32_t, std::string>>& rcv_portfolio)
            {
                bool b_valid = false;
                if (b_result) {
                    b_valid = init_portfolio(investments_type, rcv_portfolio);
                }
                if (b_valid) {
                    m_pSession->TransmitPortfolio([this](bool b_result)
                    {
                        if (b_result) {
                            m_sequence = SEQ_READY;
                        } else {
                            // 失敗した場合はBUSYのまま(スターター呼び出し側で対処)
                        }
                    });
                } else {
                    // 失敗した場合はBUSYのまま(スターター呼び出し側で対処)
                }
            });
            m_last_portfolior_investments = investments_type;
            m_sequence = SEQ_BUSY;
        } else {
            // 前回と同じ取引所なので作成不要
            m_sequence = SEQ_READY;
        }
    }

public:
    /*!
     *  @param  session     証券会社とのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const std::shared_ptr<SecuritiesSession>& session, const TradeAssistantSetting& script_mng)
    : m_session_keep_ms(utility::ToMiliSecondsFromMinute(script_mng.GetSessionKeepMinute()))
    , m_pSession(session)
    , m_sequence(SEQ_NONE)
    , m_last_portfolior_investments(INVESTMENTS_NONE)
    {
    }

    /*!
     *  @brief  株取引準備できてるか
     *  @retval true    準備OK
     */
    bool IsReady() const
    {
        return m_sequence == SEQ_READY;
    }

    /*!
     *  @brief  開始処理
     *  @param  tickCount           経過時間[ミリ秒]
     *  @param  aes_uid
     *  @param  aes_pwd
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_type    取引所種別
     *  @param  init_portfolio      ポートフォリオ初期化関数
     *  @retval true                成功
     */
    bool Start(int64_t tickCount,
               const CipherAES& aes_uid,
               const CipherAES& aes_pwd,
               const std::vector<uint32_t>& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitPortfolioFunc& init_portfolio)
    {
        if (m_sequence != SEQ_NONE && m_sequence != SEQ_READY) {
            return false; // 開始処理中
        }

        const int64_t diff_tick = tickCount - m_pSession->GetLastAccessTime();
        if (diff_tick > m_session_keep_ms) {
            // SBIへの最終アクセスからの経過時間がセッション維持時間を超えていたらログイン実行
            std::wstring uid, pwd;
            aes_uid.Decrypt(uid);
            aes_pwd.Decrypt(pwd);
            m_pSession->Login(uid, pwd, [this,
                                         monitoring_code,
                                         investments_type,
                                         init_portfolio](bool b_result, bool b_login, bool b_important_msg)
            {
                if (!b_result) {
                    // >ToDo< エラー発生時の処理(サイトの構成変更、緊急メンテナンスなど)
                    // twitter通知
                } else if (!b_login) {
                    // >ToDo< ログイン失敗時の処理(USERID/PASSWORD違い)
                    // 再入力を促す(twitter通知/トレード開始ボタン復活、machineシーケンスリセット)
                } else {
                    if (b_important_msg) {
                        // >ToDo< PCサイトに「重要なお知らせ」が来ていた場合の処理
                        // twitterへ送ってメッセージ確認を促す
                    }
                    // ポートフォリオ作成
                    CreatePortfolio(monitoring_code, investments_type, init_portfolio);
                }
            });
            m_sequence = SEQ_BUSY;
        } else {
            CreatePortfolio(monitoring_code, investments_type, init_portfolio);
        }
        return true;
    }
};

/*!
 *  @param  session     証券会社とのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockTradingStarterSbi::StockTradingStarterSbi(const std::shared_ptr<SecuritiesSession>& session,
                                               const TradeAssistantSetting& script_mng)
: StockTradingStarter()
, m_pImpl(new PIMPL(session, script_mng))
{
}
/*!
 */
StockTradingStarterSbi::~StockTradingStarterSbi()
{
}

/*!
 *  @brief  株取引準備できてるか
 */
bool StockTradingStarterSbi::IsReady() const
{
    return m_pImpl->IsReady();
}

/*!
 *  @brief  開始処理
 *  @param  tickCount           経過時間[ミリ秒]
 *  @param  aes_uid
 *  @param  aes_pwd
 *  @param  monitoring_code     監視銘柄
 *  @param  investments_type    取引所種別
 *  @param  init_portfolio      ポートフォリオ初期化関数
 */
bool StockTradingStarterSbi::Start(int64_t tickCount,
                                   const CipherAES& aes_uid,
                                   const CipherAES& aes_pwd,
                                   const std::vector<uint32_t>& monitoring_code,
                                   eStockInvestmentsType investments_type,
                                   const InitPortfolioFunc& init_portfolio)
{
    return m_pImpl->Start(tickCount, aes_uid, aes_pwd, monitoring_code, investments_type, init_portfolio);
}

} // namespace trading
