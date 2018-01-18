/*!
 *  @file   trade_struct.h
 *  @brief  トレーディング関連構造体
 *  @date   2017/12/21
 */
#pragma once

#include "trade_define.h"
#include "stock_code.h"

#include "hhmmss.h"
#include "yymmdd.h"

#include <string>
#include <vector>

namespace garnet { struct sTime; }

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

    garnet::HHMMSS m_hhmmss;//!< 時分秒
    eMode m_mode;           //!< モード

    StockTimeTableUnit()
    : m_hhmmss()
    , m_mode(CLOSED)
    {
    }
    StockTimeTableUnit(const garnet::sTime& time)
    : m_hhmmss(time)
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
    int32_t m_number;       //!< 株数
    float64 m_value;        //!< 価格
    bool m_b_leverage;      //!< 信用フラグ
    bool m_b_market_order;  //!< 成行フラグ

    eOrderType m_type;                      //!< 注文種別
    eOrderCondition m_condition;            //!< 条件
    eStockInvestmentsType m_investments;    //!< 取引所

    StockOrder()
    : m_code()
    , m_number(0)
    , m_value(0.f)
    , m_b_leverage(false)
    , m_b_market_order(false)
    , m_type(ORDER_NONE)
    , m_condition(CONDITION_NONE)
    , m_investments(INVESTMENTS_NONE)
    {
    }

    /*!
     *  @param  rcv 注文パラメータ(受信形式)
     */
    StockOrder(const RcvResponseStockOrder& rcv);

    /*!
     *  @brief  銘柄コード参照
     */
    const StockCode& RefCode() const { return m_code; }
    /*!
     *  @brief  銘柄コード取得
     */
    uint32_t GetCode() const { return m_code.GetCode(); }
    /*!
     *  @brief  株数取得 
     */
    int32_t GetNumber() const { return m_number; }
    


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
            if (m_investments != INVESTMENTS_TOKYO) {
                // 信用は東証以外ありえない
                return false;
            }
        } else {
            if (m_investments != INVESTMENTS_TOKYO && m_investments != INVESTMENTS_PTS) {
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

    /*!
     *  @note   RcvResponseStockOrderとの比較
     */
    bool operator==(const RcvResponseStockOrder& right) const;
    bool operator!=(const RcvResponseStockOrder& right) const { return !(*this == right); }

    /*!
     *  @brief  メッセージ出力用文字列生成
     *  @param[in]  order_id    注文番号
     *  @param[in]  name        銘柄名
     *  @param[in]  number      株数
     *  @param[in]  value       価格
     *  @param[out] o_str       格納先
     *  @note   twitter出力用
     */
    void BuildMessageString(int32_t order_id,
                            const std::wstring& name,
                            int32_t number,
                            float64 value,
                            std::wstring& o_str) const;
};

/*!
 *  @brief  株注文結果[受信形式]
 */
 struct RcvResponseStockOrder
{
    int32_t m_order_id;                 //!< 注文番号 管理用(SBI:グローバルっぽい/)
    int32_t m_user_order_id;            //!< 注文番号 表示用(SBI:ユーザ固有/)
    eOrderType m_type;                  //!< 注文種別
    eStockInvestmentsType m_investments;//!< 取引所種別
    uint32_t m_code;                    //!< 銘柄コード
    int32_t m_number;                   //!< 注文株数
    float64 m_value;                    //!< 注文価格
    bool m_b_leverage;                  //!< 信用フラグ

    RcvResponseStockOrder();
};

/*!
 *  @brief  株約定情報(1約定分)
 */
struct StockExecInfo
{
    int32_t m_number;           //!< 約定株数
    float64 m_value;            //!< 約定単価
    garnet::YYMMDD m_date;      //!< 約定年月日
    garnet::HHMMSS m_time;      //!< 約定時間

    StockExecInfo()
    : m_number(0)
    , m_value(0.0)
    , m_date()
    , m_time()
    {
    }

    StockExecInfo(const garnet::sTime& datetime,
                  int32_t number,
                  float64 value)
    : m_number(number)
    , m_value(value)
    , m_date(datetime)
    , m_time(datetime)
    {
    }
};

/*!
 *  @brief  株約定情報(1注文分)ヘッダ
 */
struct StockExecInfoAtOrderHeader
{
    int32_t m_user_order_id;            //!< 注文番号 表示用(SBI:ユーザ固有/)
    eOrderType  m_type;                 //!< 注文種別
    eStockInvestmentsType m_investments;//!< 約定取引所
    uint32_t m_code;                    //!< 銘柄コード
    bool m_b_leverage;                  //!< 信用フラグ
    bool m_b_complete;                  //!< 約定完了フラグ

    StockExecInfoAtOrderHeader();
};
/*!
 *  @brief  株約定情報(1注文分)
 */
struct StockExecInfoAtOrder : public StockExecInfoAtOrderHeader
{
    std::vector<StockExecInfo>  m_exec; //!< 約定情報

    /*!
     */
    StockExecInfoAtOrder()
    : StockExecInfoAtOrderHeader()
    , m_exec()
    {
    }
    /*!
     *  @brief  ヘッダだけを引き継ぐコンストラクタ
     */
    StockExecInfoAtOrder(const StockExecInfoAtOrderHeader& src)
    : StockExecInfoAtOrderHeader(src)
    , m_exec()
    {
    }
};

} // namespace trading
