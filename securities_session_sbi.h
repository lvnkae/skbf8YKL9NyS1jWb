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
     *  @brief  ポートフォリオ作成
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_type    株取引所種別
     *  @param  callback            コールバック
     *  @note   mobileサイトで監視銘柄を登録する
     */
    void CreatePortfolio(const std::unordered_set<uint32_t>& monitoring_code,
                         eStockInvestmentsType investments_type,
                         const CreatePortfolioCallback& callback) override;
    /*!
     *  @brief  ポートフォリオ転送
     *  @param  callback    コールバック
     *  @note   mobileサイトからPCサイトへの転送
     *  @note   mobileサイトは表示項目が少なすぎるのでPCサイトへ移してそちらから情報を得たい
     */
    void TransmitPortfolio(const TransmitPortfolioCallback& callback) override;
    /*!
     *  @brief  価格データ更新
     *  @param  callback    コールバック
     */
    void UpdateValueData(const UpdateValueDataCallback& callback) override;
    /*!
     *  @brief  新規売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void FreshOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) override;
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
     *  @brief  保有株式情報更新
     */
    void UpdateStockOwned() override {}
    /*!
     *  @brief  保有株売却注文
     */
    void CloseLong(const StockOrder& order) override {}
    /*!
     *  @brief  買い戻し注文
     */
    void CloseShort(const StockOrder& order) override {}

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
