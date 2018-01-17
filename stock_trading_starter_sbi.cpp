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
#include "twitter/twitter_session.h"
#include "utility/utility_datetime.h"

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

    //!< 証券会社とのセッションを無アクセスで維持できる時間[ミリ秒]
    const int64_t m_session_keep_ms;
    //!< 証券会社とのセッション
    SecuritiesSessionPtr m_pSecSession;
    //!< twitterとのセッション(メッセージ通知用)
    garnet::TwitterSessionForAuthorPtr m_pTwSession;

    eSequence m_sequence;                                   //!< シーケンス
    eStockInvestmentsType m_last_register_investments;      //!< 最後に監視銘柄を登録した取引所種別

    /*!
     *  @brief  保有銘柄取得
     *  @param  update_func 保有銘柄更新関数
     */
     void GetStockOwned(const UpdateStockHoldingsFunc& update_func)
     {
        m_pSecSession->GetStockOwned([this, update_func]
                                     (bool b_result, const SpotTradingsStockContainer& spot,
                                                     const StockPositionContainer& position,
                                                     const std::wstring& sv_date)
        {
            if (b_result) {
                m_sequence = SEQ_READY;
                update_func(spot, position, sv_date);
            }
            // 失敗した場合はBUSYのまま(スターター呼び出し側で対処)
        });
        m_sequence = SEQ_BUSY;
     }
    /*!
     *  @brief  監視銘柄コード登録
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    取引所種別
     *  @param  init_func           監視銘柄初期化関数
     *  @param  update_func         保有銘柄更新関数
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const InitMonitoringBrandFunc& init_func,
                                const UpdateStockHoldingsFunc& update_func)
    {
        // 前回と同じ取引所なので登録不要
        if (m_last_register_investments == investments_type) {
            GetStockOwned(update_func);
            return;
        }
        m_pSecSession->RegisterMonitoringCode(monitoring_code, investments_type,
                                              [this, investments_type,
                                                     init_func,
                                                     update_func]
                                              (bool b_result, const StockBrandContainer& rcv_brand)
        {
            if (b_result && init_func(investments_type, rcv_brand)) {
                GetStockOwned(update_func);
            }
            // 失敗した場合はBUSYのまま(スターター呼び出し側で対処)
        });
        m_last_register_investments = investments_type;
        m_sequence = SEQ_BUSY;
    }

public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const SecuritiesSessionPtr& sec_session,
          const garnet::TwitterSessionForAuthorPtr& tw_session,
          const TradeAssistantSetting& script_mng)
    : m_session_keep_ms(garnet::utility_datetime::ToMiliSecondsFromMinute(script_mng.GetSessionKeepMinute()))
    , m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_sequence(SEQ_NONE)
    , m_last_register_investments(INVESTMENTS_NONE)
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
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    取引所種別
     *  @param  init_func           監視銘柄初期化関数:
     *  @param  update_func         保有銘柄更新関数
     *  @retval true                成功
     */
    bool Start(int64_t tickCount,
               const garnet::CipherAES_string& aes_uid,
               const garnet::CipherAES_string& aes_pwd,
               const StockCodeContainer& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitMonitoringBrandFunc& init_func,
               const UpdateStockHoldingsFunc& update_func)
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
                                 [this, monitoring_code, investments_type,
                                  update_func, init_func]
                                  (bool b_result, bool b_login, bool b_important_msg,
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
                    // 監視銘柄登録
                    RegisterMonitoringCode(monitoring_code,
                                           investments_type,
                                           init_func, update_func);
                }
            });
            m_sequence = SEQ_BUSY;
        } else {
            RegisterMonitoringCode(monitoring_code, investments_type, init_func, update_func);
        }
        return true;
    }
};

/*!
 *  @param  sec_session 証券会社とのセッション
 *  @param  tw_session  twitterとのセッション
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
StockTradingStarterSbi::StockTradingStarterSbi(const SecuritiesSessionPtr& sec_session,
                                               const garnet::TwitterSessionForAuthorPtr& tw_session,
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
 *  @param  monitoring_code     監視銘柄コード
 *  @param  investments_type    取引所種別
 *  @param  init_func           監視銘柄初期化関数
 *  @param  update_func         保有銘柄更新関数
 */
bool StockTradingStarterSbi::Start(int64_t tickCount,
                                   const garnet::CipherAES_string& aes_uid,
                                   const garnet::CipherAES_string& aes_pwd,
                                   const StockCodeContainer& monitoring_code,
                                   eStockInvestmentsType investments_type,
                                   const InitMonitoringBrandFunc& init_func,
                                   const UpdateStockHoldingsFunc& update_func)
{
    return m_pImpl->Start(tickCount,
                          aes_uid, aes_pwd,
                          monitoring_code, investments_type, init_func, update_func);
}

} // namespace trading
