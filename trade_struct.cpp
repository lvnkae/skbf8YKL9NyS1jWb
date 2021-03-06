/*!
 *  @file   trade_struct.cpp
 *  @brief  g[fBOÖA\¢Ì
 *  @date   2017/12/21
 */
#include "trade_struct.h"
#include "trade_utility.h"

#include "twitter/twitter_session.h"
#include "utility/utility_string.h"

namespace trading
{
/*!
 *  @brief  ®Á¿R[hFÈµ
 */
const uint32_t STOCK_CODE_NONE = 0;

StockCode::StockCode()
: m_code(STOCK_CODE_NONE)
{
}

/*!
 *  @param  Á¿R[h
 */
StockCode::StockCode(uint32_t code)
: m_code(code)
{
}

/*!
 *  @brief  ÔÑæªÝè
 *  @param  period_str  ÔÑæª¶ñ
 */
bool StockPeriodOfTimeUnit::SetPeriod(const std::string& period_str)
{
    if (period_str.compare("DAY") == 0) {
        m_period = PERIOD_DAYTIME;
        return true;
    } else if (period_str.compare("NIGHT") == 0) {
        m_period = PERIOD_NIGHTTIME;
        return true;
    } else {
        m_period = PERIOD_NONE;
        return false;
    }
}

/*!
 *  @brief  [hÝè
 *  @param  mode_str    ^Ce[u[h¶ñ
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
 *  @param  rcv ¶p[^(óM`®)
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
 *  @note   RcvResponseStockOrderÆÌär
 *  @param  right   ärÎÛ
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
 *  @brief  bZ[WoÍp¶ñ¶¬
 *  @param[in]  order_id    ¶Ô
 *  @param[in]  name        Á¿¼
 *  @param[in]  number      
 *  @param[in]  value       ¿i
 *  @param[out] o_str       i[æ
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
            o_str += L"(MpVK)";
        } else {
            o_str += L"(»¨)";
        }
        break;
    case ORDER_SELL:
        if (m_b_leverage) {
            o_str += L"(MpVK)";
        } else {
            o_str += L"(»¨)";
        }
        break;
    case ORDER_CORRECT:
        o_str += L"(¶ù³)";
        break;
    case ORDER_CANCEL:
        o_str += L"(¶æÁ)";
        break;
    case ORDER_REPSELL:
        o_str += L"(MpÔÏ)";
        break;
    case ORDER_REPBUY:
        o_str += L"(MpÔÏ)";
        break;                
    }
    const int32_t DISP_ORDER_ID = 4;
    const int32_t VALUE_ORDER = trade_utility::ValueOrder();
    o_str += L" " + garnet::utility_string::ToSecretIDOrder(order_id, DISP_ORDER_ID);
    o_str += nl + std::to_wstring(code) + L" " + name;
    o_str += nl + L" " + std::to_wstring(number);
    o_str += nl + L"¿i " + garnet::utility_string::ToWstringOrder(value, VALUE_ORDER);
    if (m_b_market_order) {
        switch (m_condition)
        {
        case CONDITION_OPENING:
            o_str += L"(ñ¬)";
            break;
        case CONDITION_CLOSE:
            o_str += L"(ø¬)";
            break;
        default:
            o_str += L"(¬s)";
            break;
        }
    } else {
        switch (m_condition)
        {
        case CONDITION_OPENING:
            o_str += L"(ñw)";
            break;
        case CONDITION_CLOSE:
            o_str += L"(øw)";
            break;
        case CONDITION_UNPROMOTED:
            o_str += L"(s¬)";
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
