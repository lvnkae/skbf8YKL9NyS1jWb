/*!
 *  @file   stock_ordering_manager.h
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#pragma once

#include "trade_define.h"

#include <memory>
#include <vector>

class CipherAES;
struct HHMMSS;
class TwitterSessionForAuthor;

namespace trading
{
class SecuritiesSession;
class StockTradingTactics;
struct StockPortfolio;
class TradeAssistantSetting;

/*!
 *  @brief  株発注管理者
 */
class StockOrderingManager
{
public:
    /*!
     *  @param  sec_session 証券会社とのセッション
     *  @param  tw_session  twitterとのセッション
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    StockOrderingManager(const std::shared_ptr<SecuritiesSession>& sec_session,
                         const std::shared_ptr<TwitterSessionForAuthor>& tw_session,
                         const TradeAssistantSetting& script_mng);
    /*!
     */
    ~StockOrderingManager();

    /*!
     *  @brief  取引戦略解釈
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  hhmmss      現在時分秒
     *  @param  investments 取引所種別
     *  @param  tactics     戦略データ
     *  @param  valuedata   価格データ(1取引所分)
     *  @param  script_mng  外部設定(スクリプト)管理者
     *  @note   戦略と時刻と価格データから命令を得る
     *  @note   キューに積むだけで発注まではしない
     */
    void InterpretTactics(int64_t tickCount,
                          const HHMMSS& hhmmss,
                          eStockInvestmentsType investments,
                          const std::vector<StockTradingTactics>& tactics,
                          const std::vector<StockPortfolio>& valuedata,
                          TradeAssistantSetting& script_mng);

    /*!
     *  @brief  命令を処理する
     *  @param  aes_pwd
     */
    void IssueOrder(const CipherAES& aes_pwd);

private:
    StockOrderingManager();
    StockOrderingManager(const StockOrderingManager&);
    StockOrderingManager& operator= (const StockOrderingManager&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
