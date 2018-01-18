/*!
 *  @file   stock_code.h
 *  @brief  株式銘柄コード
 *  @date   2017/12/24
 */
#pragma once

namespace trading
{

/*!
 *  @brief  株式銘柄コード
 */
class StockCode
{
private:
    uint32_t m_code;

public:
    StockCode();
    StockCode(uint32_t code);
    ~StockCode() {}
    uint32_t GetCode() const { return m_code; }

    /*!
     *  @brief  正常チェック
     *  @retval true    正常
     */
    bool IsValid() const
    {
        const uint32_t CODE_MIN = 1300;
        const uint32_t CODE_MAX = 9999;
        return (CODE_MIN <= m_code && m_code <= CODE_MAX);
    }
};

} // namespace trading
