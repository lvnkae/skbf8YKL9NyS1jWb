/*!
 *  @file   stock_ordering_manager.h
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#pragma once

#include "trade_define.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class CipherAES;
struct HHMMSS;
class TwitterSessionForAuthor;

namespace trading
{
struct RcvStockValueData;
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
                         TradeAssistantSetting& script_mng);
    /*!
     */
    ~StockOrderingManager();

    /*!
     *  @brief  監視銘柄コード取得
     *  @param[out] dst 格納先
     */
    void GetMonitoringCode(std::unordered_set<uint32_t>& dst);
    /*!
     *  @brief  ポートフォリオ初期化
     *  @param  investments_type    取引所種別
     *  @param  rcv_portfolio       受信したポートフォリオ<銘柄コード番号, 銘柄名(utf-16)>
     *  @retval true                成功
     */
    bool InitPortfolio(eStockInvestmentsType investments_type,
                       const std::unordered_map<uint32_t, std::wstring>& rcv_portfolio);
    /*!
     *  @brief  価格データ更新
     *  @param  investments_type    取引所種別
     *  @param  senddate            価格データ送信時刻
     *  @param  rcv_valuedata       受け取った価格データ
     */
    void UpdateValueData(eStockInvestmentsType investments_type,
                         const std::wstring& sendtime,
                         const std::vector<RcvStockValueData>& rcv_valuedata);

    /*!
     *  @brief  定期更新
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  hhmmss      現在時分秒
     *  @param  investments 取引所種別
     *  @param  aes_pwd
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void Update(int64_t tickCount,
                const HHMMSS& hhmmss,
                eStockInvestmentsType investments,
                const CipherAES& aes_pwd,
                TradeAssistantSetting& script_mng);

private:
    StockOrderingManager();
    StockOrderingManager(const StockOrderingManager&);
    StockOrderingManager& operator= (const StockOrderingManager&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
