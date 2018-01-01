/*!
 *  @file   trade_struct.h
 *  @brief  トレーディング関連構造体
 *  @date   2017/12/21
 */
#pragma once

#include "hhmmss.h"
#include "trade_define.h"
#include "stock_code.h"
#include <string>

namespace trading
{

struct RcvResponseStockOrder;

/*!
 *  @brief  株取引タイムテーブル(一コマ分)
 */
struct StockTimeTableUnit
{
    enum eMode
    {
        CLOSED, //!< 閉場(何もしない)
        IDLE,   //!< 次MODE準備時間
        TOKYO,  //!< 金融証券取引所で売買
        PTS,    //!< 私設取引所で売買
    };

    HHMMSS m_hhmmss;//!< 時分秒
    eMode m_mode;   //!< モード

    StockTimeTableUnit()
    : m_hhmmss()
    , m_mode(CLOSED)
    {
    }
    StockTimeTableUnit(int32_t h, int32_t m, int32_t s)
    : m_hhmmss(h, m, s)
    , m_mode(CLOSED)
    {
    }

    /*!
     *  @brief  モード文字列(スクリプト用)を取引所種別に変換
     *  @param  mode_str    タイムテーブルモード文字列
     */
    static eStockInvestmentsType ToInvestmentsTypeFromMode(eMode tt_mode)
    {
        if (tt_mode == StockTimeTableUnit::TOKYO) {
            return INVESTMENTS_TOKYO;
        } else if (tt_mode == StockTimeTableUnit::PTS) {
            return INVESTMENTS_PTS;
        } else {
            // 名証/福証/札証は対応しない
            return INVESTMENTS_NONE;
        }
    }

    /*!
     *  @brief  モード設定
     *  @param  mode_str    タイムテーブルモード文字列
     *  @retval true    成功
     */
    bool SetMode(const std::string& mode_str);

    /*!
     *  @brief  比較処理
     *  @param  right   右辺値
     *  @retval true    右辺値が大きい
     */
    bool operator<(const StockTimeTableUnit& right) const {
        return m_hhmmss < right.m_hhmmss;
    }
};

/*!
 *  @brief  株注文パラメータ
 */
 struct StockOrder
{
    StockCode m_code;       //!< 銘柄コード
    uint32_t m_number;      //!< 株数
    float64 m_value;        //!< 価格
    bool m_b_leverage;      //!< 信用フラグ
    bool m_b_market_order;  //!< 成行フラグ

    eOrderType m_type;                      //!< 注文種別
    eOrderConditon m_condition;             //!< 条件
    eStockInvestmentsType m_investiments;   //!< 取引所

    StockOrder()
    : m_code()
    , m_number(0)
    , m_value(0.f)
    , m_b_leverage(false)
    , m_b_market_order(false)
    , m_type(ORDER_NONE)
    , m_condition(CONDITION_NONE)
    , m_investiments(INVESTMENTS_NONE)
    {
    }

    /*!
     *  @param  rcv 注文パラメータ(受信形式)
     */
    StockOrder(const RcvResponseStockOrder& rcv);

    /*!
     *  @brief  注文の正常チェック
     *  @retval true    正常
     */
    bool IsValid() const
    {
        if (!m_code.IsValid()) {
            return false;   // 不正銘柄コード
        }
        if (m_number == 0) {
            return false;   // 株数指定なし
        }
        if (m_b_market_order) {
            if (m_value > 0.0) {
                // 成行 + 価格正(スクリプトでは価格マイナス=成行指定)
                return false;
            }
            if (m_condition == CONDITION_UNPROMOTED) {
                // 成行き + 不成
                return false;
            }
        } else {
            if (static_cast<int32_t>(m_value) < 1) {
                // 非成行き + 価格0円台と負数はありえない
                return false;
            }
        }
        if (m_b_leverage) {
            if (m_investiments != INVESTMENTS_TOKYO) {
                // 信用は東証以外ありえない
                return false;
            }
        } else {
            if (m_investiments != INVESTMENTS_TOKYO && m_investiments != INVESTMENTS_PTS) {
                // 取引所不正(未指定または未対応)
                return false;
            }
        }
        if (m_type == ORDER_NONE) {
            // 注文種別不正
            return false;
        }
        //
        return true;
    }
};

/*!
 *  @brief  株注文結果[受信形式]
 */
 struct RcvResponseStockOrder
{
    int32_t m_order_id;                 //!< 注文番号
    eOrderType m_type;                  //!< 注文種別
    eStockInvestmentsType m_investments;//!< 取引所種別
    uint32_t m_code;                    //!< 銘柄コード
    uint32_t m_number;                  //!< 注文株数
    float64 m_value;                    //!< 注文価格
    bool m_b_leverage;                  //!< 信用フラグ

    RcvResponseStockOrder()
    : m_order_id(0)
    , m_type(ORDER_NONE)
    , m_investments(INVESTMENTS_NONE)
    , m_code(0)
    , m_number(0)
    , m_value(0.0)
    {
    }
};

} // namespace trading
