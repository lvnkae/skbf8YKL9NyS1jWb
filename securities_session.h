/*!
 *  @file   securities_session.h
 *  @brief  証券会社サイトとのセッション管理
 *  @date   2017/05/05
 */
#pragma once

#include "trade_container.h"
#include "trade_define.h"

#include <functional>
#include <string>
#include <vector>

namespace garnet { struct YYMMDD; }

namespace trading
{

struct StockExecInfoAtOrder;
struct StockOrder;
struct RcvStockValueData;
struct RcvResponseStockOrder;

class SecuritiesSession
{
public:
    typedef std::function<void (bool b_result, bool, bool,
                                const std::wstring& sv_date)> LoginCallback;
    typedef std::function<void (bool b_result,
                                const StockBrandContainer&)> RegisterMonitoringCodeCallback;
    typedef std::function<void (bool b_result,
                                const SpotTradingsStockContainer&,
                                const StockPositionContainer&,
                                const std::wstring& sv_date)> GetStockOwnedCallback;
    typedef std::function<void (bool b_result,
                                const std::vector<RcvStockValueData>&,
                                const std::wstring& sv_date)> UpdateValueDataCallback;
    typedef std::function<void (bool b_result,
                                const std::vector<StockExecInfoAtOrder>&)> UpdateStockExecInfoCallback;
    typedef std::function<void (bool b_result)> UpdateMarginCallback;
    typedef std::function<void (bool b_result,
                                const RcvResponseStockOrder&,
                                const std::wstring& sv_date)> OrderCallback;

    SecuritiesSession();
    virtual ~SecuritiesSession();


    /*!
     *  @breif  ログイン
     *  @param  uid
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback) = 0;

    /*!
     *  @brief  監視銘柄コード登録
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_type    株取引所種別
     *  @param  callback            コールバック
     */
    virtual void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                        eStockInvestmentsType investments_type,
                                        const RegisterMonitoringCodeCallback& callback) = 0;
    /*!
     *  @brief  保有株式情報取得
     *  @param  callback    コールバック
     */
    virtual void GetStockOwned(const GetStockOwnedCallback& callback) = 0;

    /*!
     *  @brief  監視銘柄価格データ取得
     *  @param  callback    コールバック
     */
    virtual void UpdateValueData(const UpdateValueDataCallback& callback) = 0;
    /*!
     *  @brief  約定情報取得取得
     *  @param  callback    コールバック
     */
    virtual void UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback) = 0;
    /*!
     *  @brief  余力取得
     *  @param  callback    コールバック
     */
    virtual void UpdateMargin(const UpdateMarginCallback& callback) = 0;

    /*!
     *  @brief  売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     *  @note   現物売買/信用新規売買
     */
    virtual void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) = 0;
    /*!
     *  @brief  信用返済注文
     *  @param  t_yymmdd    建日
     *  @param  t_value     建単価
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                        const StockOrder& order,
                                        const std::wstring& pwd,
                                        const OrderCallback& callback) = 0;
    /*!
     *  @brief  注文訂正
     *  @param  order_id    注文番号(管理用)
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) = 0;
    /*!
     *  @brief  注文取消
     *  @param  order_id    注文番号(管理用)
     *  @param  pwd
     *  @param  callback    コールバック
     */
    virtual void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback) = 0;


    /*!
     *  @brief  証券会社サイト最終アクセス時刻取得
     *  @return アクセス時刻
     */
    virtual int64_t GetLastAccessTime() const = 0;

private:
    SecuritiesSession(const SecuritiesSession&);
    SecuritiesSession(SecuritiesSession&&);
    SecuritiesSession& operator= (const SecuritiesSession&);
};

} // namespace trading
