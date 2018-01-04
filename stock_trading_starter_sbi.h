/*!
 *  @file   stock_trading_starter_sbi.h
 *  @brief  株取引スタート係：SBI用
 *  @date   2017/12/20
 *  @note   ログイン→監視銘柄登録の順で実行
 */
#pragma once

#include "stock_trading_starter.h"
#include <memory>

class TwitterSessionForAuthor;

namespace trading
{
class SecuritiesSession;
class TradeAssistantSetting;

class StockTradingStarterSbi : public StockTradingStarter
{
public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    StockTradingStarterSbi(const std::shared_ptr<SecuritiesSession>& sec_session,
                           const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
                           const TradeAssistantSetting& script_mng);
    /*!
     */
    ~StockTradingStarterSbi();

    /*!
     *  @brief  株取引準備できてるか
     *  @retval true    準備OK
     */
    bool IsReady() const override;

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
               const InitPortfolioFunc& init_portfolio);

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
