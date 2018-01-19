/*!
 *  @file   stock_portfolio.cpp
 *  @brief  ���Ď������f�[�^
 *  @date   2017/12/25
 */

#include "stock_portfolio.h"
#include "trade_utility.h"

#include "garnet_time.h"
#include "utility/utility_string.h"

#include <fstream>

namespace trading
{

/*!
 *  @brief  ���i�f�[�^�X�V
 *  @param  src     ���i�f�[�^
 *  @param  date    �擾����(�T�[�o���Ԃ��g��)
 */
void StockValueData::UpdateValueData(const RcvStockValueData& src, const garnet::sTime& date)
{
    m_open = src.m_open;
    m_high = src.m_high;
    m_low  = src.m_low;
    m_close = src.m_close;

    if (m_value_data.empty()) {
        // �󂾂�����Ȃ�ł��o�^
    } else {
        // ��łȂ���΁A�O��Əo�������قȂ��Ă����ꍇ�����o�^
        // ���u�ȏ�v�ɂ���Əo���������Z�b�g�������PTS�����W�ł��Ȃ�
        const stockValue& last = m_value_data.back();
        if (last.m_volume == src.m_volume) {
            return;
        }
    }
    m_value_data.emplace_back(date, src.m_value, src.m_volume);
};

/*!
 *  @brief  ���O�o��
 *  @param  filename    �o�̓t�@�C����(�p�X�܂�)
 */
void StockValueData::OutputLog(const std::string& filename) const
{
    std::ofstream outputfile(filename.c_str());

    using garnet::utility_string::ToStringOrder;
    const int32_t VORDER = trade_utility::ValueOrder();

    outputfile << std::to_string(m_code.GetCode()).c_str() << ",";
    outputfile << ToStringOrder(m_open ,  VORDER).c_str() << ",";
    outputfile << ToStringOrder(m_high ,  VORDER).c_str() << ",";
    outputfile << ToStringOrder(m_low  ,  VORDER).c_str() << ",";
    outputfile << ToStringOrder(m_close,  VORDER).c_str() << std::endl;

    for (const auto& vu: m_value_data) {
        outputfile << vu.m_hhmmss.to_delim_string().c_str() << ",";
        outputfile << ToStringOrder(vu.m_value, VORDER).c_str() << ",";
        outputfile << vu.m_volume << std::endl;
    }

    outputfile.close();
}

RcvStockValueData::RcvStockValueData()
: m_code(StockCode().GetCode())
, m_value(0.f)
, m_open(0.f)
, m_high(0.f)
, m_low(0.f)
, m_close(0.f)
, m_volume(0)
{
}

} // namespace trading
