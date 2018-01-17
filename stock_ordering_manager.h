/*!
 *  @file   stock_ordering_manager.h
 *  @brief  株発注管理者
 *  @date   2017/12/26
 */
#pragma once

#include "securities_session_fwd.h"
#include "trade_container.h"
#include "trade_define.h"

#include "twitter/twitter_session_fwd.h"

#include <memory>
#include <vector>

namespace garnet
{
class CipherAES_string;
struct HHMMSS;
struct YYMMDD;
} // namespace garnet

namespace trading
{
struct RcvStockValueData;
struct StockExecInfoAtOrder;
class StockTradingTactics;
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
    StockOrderingManager(const SecuritiesSessionPtr& sec_session,
                         const garnet::TwitterSessionForAuthorPtr& tw_session,
                         TradeAssistantSetting& script_mng);
    /*!
     */
    ~StockOrderingManager();

    /*!
     *  @brief  証券会社からの返答を待ってるか
     *  @retval true    発注結果待ちしてる
     */
    bool IsInWaitMessageFromSecurities() const;

    /*!
     *  @brief  監視銘柄コード取得
     *  @param[out] dst 格納先
     */
    void GetMonitoringCode(StockCodeContainer& dst);
    /*!
     *  @brief  監視銘柄初期化
     *  @param  investments_type    取引所種別
     *  @param  rcv_brand_data      受信した監視銘柄群
     *  @retval true                成功
     */
    bool InitMonitoringBrand(eStockInvestmentsType investments_type,
                             const StockBrandContainer& rcv_brand_data);
    /*!
     *  @brief  監視銘柄情報出力
     *  @param  log_dir 出力ディレクトリ
     *  @param  date    年月日
     */
    void OutputMonitoringLog(const std::string& log_dir,
                             const garnet::YYMMDD& date);

    /*!
     *  @brief  保有銘柄更新
     *  @param  spot        現物保有株
     *  @param  position    信用保有株
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position);

    /*!
     *  @brief  監視銘柄価格データ更新
     *  @param  investments_type    取引所種別
     *  @param  senddate            価格データ送信時刻
     *  @param  rcv_valuedata       受け取った価格データ
     */
    void UpdateValueData(eStockInvestmentsType investments_type,
                         const std::wstring& sendtime,
                         const std::vector<RcvStockValueData>& rcv_valuedata);
    /*!
     *  @brief  当日約定情報更新
     *  @param  rcv_info    受け取った約定情報
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info);

    /*!
     *  @brief  Update関数
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  now_time    現在時分秒
     *  @param  sec_time    現セクション開始時刻
     *  @param  investments 取引所種別
     *  @param  aes_pwd
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    void Update(int64_t tickCount,
                const garnet::HHMMSS& now_time,
                const garnet::HHMMSS& sec_time,
                eStockInvestmentsType investments,
                const garnet::CipherAES_string& aes_pwd,
                TradeAssistantSetting& script_mng);

private:
    StockOrderingManager();
    StockOrderingManager(const StockOrderingManager&);
    StockOrderingManager(StockOrderingManager&&);
    StockOrderingManager& operator= (const StockOrderingManager&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
