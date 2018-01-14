/*!
 *  @file   stock_trading_starter.h
 *  @brief  株取引スタート係
 *  @date   2017/12/20
 */
#pragma once

#include "trade_container.h"
#include "trade_define.h"

#include <functional>
#include <vector>

namespace garnet { class CipherAES_string; }

namespace trading
{
class StockTradingStarter
{
public:
    typedef std::function<bool(eStockInvestmentsType, const StockBrandContainer&)> InitMonitoringBrandFunc;
    typedef std::function<void(const SpotTradingsStockContainer&, const StockPositionContainer&)> UpdateStockHoldingsFunc;

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
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    取引所種別
     *  @param  init_func           監視銘柄初期化関数
     *  @param  update_func         保有銘柄更新関数
     *  @retval true                成功
     */
    virtual bool Start(int64_t tickCount,
                       const garnet::CipherAES_string& aes_uid,
                       const garnet::CipherAES_string& aes_pwd,
                       const StockCodeContainer& monitoring_code,
                       eStockInvestmentsType investments_type,
                       const InitMonitoringBrandFunc& init_func,
                       const UpdateStockHoldingsFunc& update_func) = 0;

private:
    StockTradingStarter(const StockTradingStarter&);
    StockTradingStarter(StockTradingStarter&&);
    StockTradingStarter& operator= (const StockTradingStarter&);
};

} // namespace trading
