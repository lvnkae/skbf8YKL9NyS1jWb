/*!
 *  @file   stock_trading_starter_sbi.h
 *  @brief  株取引スタート係：SBI用
 *  @date   2017/12/20
 *  @note   ログイン→監視銘柄登録の順で実行
 */
#pragma once

#include "stock_trading_starter.h"
#include "securities_session_fwd.h"

#include "twitter/twitter_session_fwd.h"

#include <memory>

namespace trading
{
class TradeAssistantSetting;

class StockTradingStarterSbi : public StockTradingStarter
{
public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    StockTradingStarterSbi(const SecuritiesSessionPtr& sec_session,
                           const garnet::TwitterSessionForAuthorPtr& tw_session,
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
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    取引所種別
     *  @param  init_func           監視銘柄初期化関数
     *  @param  update_func         保有銘柄更新関数
     *  @retval true                成功
     */
    bool Start(int64_t tickCount,
               const garnet::CipherAES_string& aes_uid,
               const garnet::CipherAES_string& aes_pwd,
               const StockCodeContainer& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitMonitoringBrandFunc& init_func,
               const UpdateStockHoldingsFunc& update_func);

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
