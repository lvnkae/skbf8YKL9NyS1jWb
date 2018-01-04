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
#include "twitter_session.h"
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

    const int64_t m_session_keep_ms;                        //!< 証券会社とのセッションを無アクセスで維持できる時間(ms)
    std::shared_ptr<SecuritiesSession> m_pSecSession;       //!< 証券会社とのセッション
    std::shared_ptr<TwitterSessionForAuthor> m_pTwSession;  //!< twitterとのセッション(メッセージ通知用)

    eSequence m_sequence;                                   //!< シーケンス
    eStockInvestmentsType m_last_portfolior_investments;    //!< 最後に作ったポートフォリオの対象取引所種別

    /*!
     *  @brief  ポートフォリオ作成
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_type    取引所種別
     *  @param  init_portfolio      ポートフォリオ初期化関数
     */
    void CreatePortfolio(const std::unordered_set<uint32_t>& monitoring_code,
                         eStockInvestmentsType investments_type,
                         const InitPortfolioFunc& init_portfolio)
    {
        if (m_last_portfolior_investments != investments_type) {
            m_pSecSession->CreatePortfolio(monitoring_code,
                                           investments_type,
                                           [this, 
                                            investments_type,
                                            init_portfolio](bool b_result,
                                                            const std::unordered_map<uint32_t, std::wstring>& rcv_portfolio)
            {
                bool b_valid = false;
                if (b_result) {
                    b_valid = init_portfolio(investments_type, rcv_portfolio);
                }
                if (b_valid) {
                    m_pSecSession->TransmitPortfolio([this](bool b_result)
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
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const std::shared_ptr<SecuritiesSession>& sec_session,
          const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
          const TradeAssistantSetting& script_mng)
    : m_session_keep_ms(utility::ToMiliSecondsFromMinute(script_mng.GetSessionKeepMinute()))
    , m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
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
               const std::unordered_set<uint32_t>& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitPortfolioFunc& init_portfolio)
    {
        if (m_sequence != SEQ_NONE && m_sequence != SEQ_READY) {
            return false; // 開始処理中
        }

        const int64_t diff_tick = tickCount - m_pSecSession->GetLastAccessTime();
        if (diff_tick > m_session_keep_ms) {
            // SBIへの最終アクセスからの経過時間がセッション維持時間を超えていたらログイン実行
            std::wstring uid, pwd;
            aes_uid.Decrypt(uid);
            aes_pwd.Decrypt(pwd);
            m_pSecSession->Login(uid, pwd,
                                 [this, monitoring_code,
                                        investments_type,
                                        init_portfolio](bool b_result,
                                                        bool b_login,
                                                        bool b_important_msg,
                                                        const std::wstring& sv_date)
            {
                if (!b_result) {
                    m_pTwSession->Tweet(sv_date, L"ログインエラー。緊急メンテナンス中かもしれません。");
                } else if (!b_login) {
                    m_pTwSession->Tweet(sv_date, L"ログインできませんでした。IDまたはパスワードが違います。");
                    // >ToDo< 再入力できるようにする(トレード開始ボタン復活、machineシーケンスリセット)
                } else {
                    if (b_important_msg) {
                        m_pTwSession->Tweet(sv_date, L"ログインしました。SBIからの重要なお知らせがあります。");
                    } else {
                        m_pTwSession->Tweet(sv_date, L"ログインしました");
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
 *  @param  sec_session 証券会社とのセッション
 *  @param  tw_session  twitterとのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockTradingStarterSbi::StockTradingStarterSbi(const std::shared_ptr<SecuritiesSession>& sec_session,
                                               const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
                                               const TradeAssistantSetting& script_mng)
: StockTradingStarter()
, m_pImpl(new PIMPL(sec_session, tw_session, script_mng))
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
                                   const std::unordered_set<uint32_t>& monitoring_code,
                                   eStockInvestmentsType investments_type,
                                   const InitPortfolioFunc& init_portfolio)
{
    return m_pImpl->Start(tickCount, aes_uid, aes_pwd, monitoring_code, investments_type, init_portfolio);
}

} // namespace trading
