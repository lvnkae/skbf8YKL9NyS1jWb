/*!
 *  @file   trade_struct.cpp
 *  @brief  トレーディング関連構造体
 *  @date   2017/12/21
 */
#include "trade_struct.h"
#include "trade_utility.h"

#include "twitter/twitter_session.h"
#include "utility/utility_string.h"

namespace trading
{
/*!
 *  @brief  株式銘柄コード：なし
 */
const uint32_t STOCK_CODE_NONE = 0;

StockCode::StockCode()
: m_code(STOCK_CODE_NONE)
{
}

/*!
 *  @param  銘柄コード
 */
StockCode::StockCode(uint32_t code)
: m_code(code)
{
}

/*!
 *  @brief  モード設定
 *  @param  mode_str    タイムテーブルモード文字列
 */
bool StockTimeTableUnit::SetMode(const std::string& mode_str)
{
    if (mode_str.compare("CLOSED") == 0) {
        m_mode = CLOSED;
    } else if (mode_str.compare("IDLE") == 0) {
        m_mode = IDLE;
    } else if (mode_str.compare("TOKYO") == 0) {
        m_mode = TOKYO;
    } else if (mode_str.compare("PTS") == 0) {
        m_mode = PTS;
    } else {
        return false;
    }
    return true;
}

/*!
 *  @param  rcv 注文パラメータ(受信形式)
 */
StockOrder::StockOrder(const RcvResponseStockOrder& rcv)
: m_code(rcv.m_code)
, m_number(rcv.m_number)
, m_value(rcv.m_value)
, m_b_leverage(rcv.m_b_leverage)
, m_b_market_order(false)
, m_type(rcv.m_type)
, m_condition(CONDITION_NONE)
, m_investments(rcv.m_investments)
{
}

/*!
 *  @note   RcvResponseStockOrderとの比較
 *  @param  right   比較対象
 */
bool StockOrder::operator==(const RcvResponseStockOrder& right) const
{
    return (m_code.GetCode() == right.m_code &&
            m_type == right.m_type &&
            m_investments == right.m_investments &&
            ((m_b_leverage && right.m_b_leverage) || (!m_b_leverage && !right.m_b_leverage)) &&
            trade_utility::same_value(m_value, right.m_value) &&
            m_number == right.m_number);
}

/*!
 *  @brief  メッセージ出力用文字列生成
 *  @param[in]  order_id    注文番号
 *  @param[in]  name        銘柄名
 *  @param[in]  number      株数
 *  @param[in]  value       価格
 *  @param[out] o_str       格納先
 */
void StockOrder::BuildMessageString(int32_t order_id,
                                    const std::wstring& name,
                                    int32_t number,
                                    float64 value,
                                    std::wstring& o_str) const
{
    const std::wstring nl(std::move(garnet::twitter::GetNewlineString()));
    const uint32_t code = GetCode();
    switch (m_type)
    {
    case ORDER_BUY:
        if (m_b_leverage) {
            o_str += L"(信用新規買)";
        } else {
            o_str += L"(現物買)";
        }
        break;
    case ORDER_SELL:
        if (m_b_leverage) {
            o_str += L"(信用新規売)";
        } else {
            o_str += L"(現物売)";
        }
        break;
    case ORDER_CORRECT:
        o_str += L"(注文訂正)";
        break;
    case ORDER_CANCEL:
        o_str += L"(注文取消)";
        break;
    case ORDER_REPSELL:
        o_str += L"(信用返済売)";
        break;
    case ORDER_REPBUY:
        o_str += L"(信用返済買)";
        break;                
    }
    const int32_t DISP_ORDER_ID = 4;
    const int32_t VALUE_ORDER = trade_utility::ValueOrder();
    o_str += L" " + garnet::utility_string::ToSecretIDOrder(order_id, DISP_ORDER_ID);
    o_str += nl + std::to_wstring(code) + L" " + name;
    o_str += nl + L"株数 " + std::to_wstring(number);
    o_str += nl + L"価格 " + garnet::utility_string::ToWstringOrder(value, VALUE_ORDER);
    if (m_b_market_order) {
        switch (m_condition)
        {
        case CONDITION_OPENING:
            o_str += L"(寄成)";
            break;
        case CONDITION_CLOSE:
            o_str += L"(引成)";
            break;
        default:
            o_str += L"(成行)";
            break;
        }
    } else {
        switch (m_condition)
        {
        case CONDITION_OPENING:
            o_str += L"(寄指)";
            break;
        case CONDITION_CLOSE:
            o_str += L"(引指)";
            break;
        case CONDITION_UNPROMOTED:
            o_str += L"(不成)";
            break;
        default:
            break;
        }
    }
}


RcvResponseStockOrder::RcvResponseStockOrder()
: m_order_id(trade_utility::BlankOrderID())
, m_user_order_id(trade_utility::BlankOrderID())
, m_type(ORDER_NONE)
, m_investments(INVESTMENTS_NONE)
, m_code(StockCode().GetCode())
, m_number(0)
, m_value(0.0)
, m_b_leverage(false)
{
}

StockExecInfoAtOrderHeader::StockExecInfoAtOrderHeader()
: m_user_order_id(trade_utility::BlankOrderID())
, m_type(ORDER_NONE)
, m_investments(INVESTMENTS_NONE)
, m_code(StockCode().GetCode())
, m_b_leverage(false)
, m_b_complete(false)
{
}

} // namespace trading
