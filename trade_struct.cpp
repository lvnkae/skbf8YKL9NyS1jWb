/*!
 *  @file   trade_struct.cpp
 *  @brief  トレーディング関連構造体
 *  @date   2017/12/21
 */
#include "trade_struct.h"

namespace trading
{
/*!
 *  @brief  株式銘柄コード：なし
 */
const uint32_t STOCK_CODE_NONE = 0;

/*!
 *  @brief
 */
StockCode::StockCode()
: m_code(STOCK_CODE_NONE)
{
}

/*!
*  @brief
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
, m_investiments(rcv.m_investments)
{
}

} // namespace trading
