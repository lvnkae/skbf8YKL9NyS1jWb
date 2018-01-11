/*!
 *  @file   securities_session_sbi.h
 *  @brief  SBI証券サイトとのセッション管理
 *  @date   2017/05/05
 */
#pragma once

#include "securities_session.h"

#include <memory>

namespace trading
{

struct StockOrder;
class TradeAssistantSetting;

class SecuritiesSessionSbi : public SecuritiesSession
{
public:
    /*!
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    SecuritiesSessionSbi(const TradeAssistantSetting& script_mng);
    /*!
     */
    ~SecuritiesSessionSbi();

    /*!
     *  @breif  ログイン
     *  @param  uid
     *  @param  pwd
     *  @param  callback    コールバック
     *  @note   mobile→PCの順にログイン
     */
    void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback) override;

    /*!
     *  @brief  監視銘柄コード登録
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    株取引所種別
     *  @param  callback            コールバック
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const RegisterMonitoringCodeCallback& callback) override;
    /*!
     *  @brief  保有株式情報取得
     */
    void GetStockOwned(const GetStockOwnedCallback& callback) override;

    /*!
     *  @brief  監視銘柄価格データ取得
     *  @param  callback    コールバック
     */
    void UpdateValueData(const UpdateValueDataCallback& callback) override;
    /*!
     *  @brief  約定情報取得取得
     */
    void UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback) override;

    /*!
     *  @brief  売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     *  @note   現物売買/信用新規売買
     */
    void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) override;
    /*!
     *  @brief  信用返済注文
     *  @param  t_yymmdd    建日
     *  @param  t_value     建単価
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                const StockOrder& order,
                                const std::wstring& pwd,
                                const OrderCallback& callback) override;
    /*!
     *  @brief  注文訂正
     *  @param  order_id    注文番号(管理用)
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) override;
    /*!
     *  @brief  注文取消
     *  @param  order_id    注文番号(管理用)
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback) override;


    /*!
     *  @brief  証券会社サイト最終アクセス時刻取得
     *  @return アクセス時刻(tickCount)
     *  @note   mobileとPCでより古い方を返す
     */
    int64_t GetLastAccessTime() const override;

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
