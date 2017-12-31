/*!
 *  @file   stock_trading_starter.h
 *  @brief  株取引スタート係
 *  @date   2017/12/20
 */
#pragma once

#include "trade_define.h"

#include <functional>
#include <vector>

class CipherAES;

namespace trading
{
class StockTradingStarter
{
public:
    typedef std::function<bool(eStockInvestmentsType, const std::vector<std::pair<uint32_t, std::string>>& rcv_portfolio)> InitPortfolioFunc;

    StockTradingStarter();
    ~StockTradingStarter();

    /*!
     *  @brief  株取引準備できてるか
     *  @retval true    準備OK
     */
    virtual bool IsReady() const = 0;

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
    virtual bool Start(int64_t tickCount,
                       const CipherAES& aes_uid,
                       const CipherAES& aes_pwd,
                       const std::vector<uint32_t>& monitoring_code,
                       eStockInvestmentsType investments_type,
                       const InitPortfolioFunc& init_portfolio) = 0;

private:
    StockTradingStarter(const StockTradingStarter&);
    StockTradingStarter& operator= (const StockTradingStarter&);
};

} // namespace trading
