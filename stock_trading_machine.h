/*!
 *  @file   stock_trading_machine.h
 *  @brief  売買処理：株
 *  @date   2017/05/05
 */
#pragma once

#include "trading_machine.h"

#include "twitter/twitter_session_fwd.h"

#include <memory>

namespace trading
{

class StockTradingMachine : public TradingMachine
{
public:
    /*!
     *  @param  script_mng  外部設定(スクリプト)管理者
     *  @param  tw_session  twitterとのセッション
     */
    StockTradingMachine(const TradeAssistantSetting& script_mng,
                        const garnet::TwitterSessionForAuthorPtr& tw_session);
    /*!
     */
    ~StockTradingMachine();

    /*!
     *  @brief  トレード開始できるか？
     *  @retval true    開始できる
     */
    bool IsReady() const override;

    /*!
     *  @brief  トレード開始
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(int64_t tickCount,
               const std::wstring& uid,
               const std::wstring& pwd,
               const std::wstring& pwd_sub) override;

    /*!
     *  @brief  Update関数
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[in]  script_mng  外部設定(スクリプト)管理者
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update(int64_t tickCount,
                TradeAssistantSetting& script_mng,
                UpdateMessage& o_message) override;

private:
    StockTradingMachine();

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
