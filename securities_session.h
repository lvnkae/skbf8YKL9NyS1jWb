/*!
 *  @file   securities_session.h
 *  @brief  証券会社サイトとのセッション管理
 *  @date   2017/05/05
 */
#pragma once

#include "trade_define.h"
#include <functional>
#include <string>
#include <vector>

namespace trading
{

struct StockOrder;
struct RcvStockValueData;
struct RcvResponseStockOrder;

class SecuritiesSession
{
public:
    typedef std::function<void (bool b_result, bool b_login, bool b_important_msg, const std::wstring&)> LoginCallback;
    typedef std::function<void (bool b_result, const std::vector<std::pair<uint32_t, std::string>>&)> CreatePortfolioCallback;
    typedef std::function<void (bool b_result)> TransmitPortfolioCallback;
    typedef std::function<void (bool b_result, const std::wstring&, const std::vector<RcvStockValueData>&)> UpdateValueDataCallback;
    typedef std::function<void (bool b_result, const RcvResponseStockOrder&, const std::wstring&)> OrderCallback;

    /*!
     *  @brief
     */
    SecuritiesSession();
    /*!
     *  @brief
     */
    virtual ~SecuritiesSession();


    /*!
     *  @breif  ログイン
     *  @param  uid
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback) = 0;
    /*!
     *  @brief  ポートフォリオ作成
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_type    株取引所種別
     *  @param  callback            コールバック
     */
    virtual void CreatePortfolio(const std::vector<uint32_t>& monitoring_code,
                                 eStockInvestmentsType investments_type,
                                 const CreatePortfolioCallback& callback) = 0;
    /*!
     *  @brief  ポートフォリオ転送
     *  @param  callback    コールバック
     *  @note   SBIでしか使わないはず
     */
    virtual void TransmitPortfolio(const TransmitPortfolioCallback& callback) = 0;
    /*!
     *  @brief  価格データ更新
     *  @param  callback    コールバック
     */
    virtual void UpdateValueData(const UpdateValueDataCallback& callback) = 0;
    /*!
     *  @brief  新規売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void FreshOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) = 0;

    /*!
     *  @brief  保有株式情報更新
     */
    virtual void UpdateStockOwned() = 0;
    /*!
     *  @brief  保有株売却注文
     */
    virtual void CloseLong(const StockOrder& order) = 0;
    /*!
     *  @brief  買い戻し注文
     */
    virtual void CloseShort(const StockOrder& order) = 0;

    /*!
     *  @brief  証券会社サイト最終アクセス時刻取得
     *  @return アクセス時刻
     */
    virtual int64_t GetLastAccessTime() const = 0;

private:
    SecuritiesSession(const SecuritiesSession&);
    SecuritiesSession& operator= (const SecuritiesSession&);
};

} // namespace trading
