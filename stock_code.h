/*!
 *  @file   stock_code.h
 *  @brief  ���������R�[�h
 *  @date   2017/12/24
 */
#pragma once

namespace trading
{

/*!
 *  @brief  ���������R�[�h
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
     *  @brief  ����`�F�b�N
     *  @retval true    ����
     */
    bool IsValid() const
    {
        const uint32_t CODE_MIN = 1300;
        const uint32_t CODE_MAX = 9999;
        return (CODE_MIN <= m_code && m_code <= CODE_MAX);
    }
};

} // namespace trading
