/*!
 *  @file   stock_holdings_keeper.cpp
 *  @brief  ���ۗL�����Ǘ�
 *  @date   2018/01/09
 */

#include "stock_holdings_keeper.h"
#include "stock_holdings.h"

#include "stock_trading_command.h"
#include "trade_struct.h"
#include "trade_utility.h"

#include "hhmmss.h"
#include "yymmdd.h"

#include <algorithm>
#include <unordered_map>

namespace trading
{

class StockHoldingsKeeper::PIMPL
{
private:
    /*!
     *  @brief  �M�p�ۗL��(����)�f�[�^<�����R�[�h, <���ʌŗLID, �ۗL����>>
     *  @note   �ŗLID�͓�����蒍���Ƃ̕R�t���p(�O�c�Ɠ��ȑO�̋ʂ�ID�Ȃ�)
     */
    typedef std::unordered_map<uint32_t, std::list<std::pair<int32_t, StockPosition>>> StockPositionData;
    /*!
     *  @brief  �����ۗL���f�[�^<�����R�[�h, ����>
     *  @note   �����͖����Ɗ����ł̂݊Ǘ�����
     *  @note   (�����t������P�����w�肵�Ă̔����͏o���Ȃ��̂�)
     */
    typedef SpotTradingsStockContainer SpotTradingStockData;

    //! �����ۗL��
    SpotTradingStockData m_spot_data;
    //! �M�p�ۗL��(����)
    StockPositionData m_position_data;

    //! ���ʌŗLID���s��
    uint32_t m_position_id_source;
    //! ������蒍��<�����ԍ�(�\���p), �����>
    std::unordered_map<int32_t, StockExecInfoAtOrder> m_today_exec_order;

    /*!
     *  @brief  ���ʌŗLID���s
     */
    uint32_t IssuePositionID()
    {
        return ++m_position_id_source;
    }

    /*!
     *  @brief  �����������ۗL���ɔ��f
     *  @param  diff    �����(����) 1������
     */
    void ReflectExecInfoToSpot(const StockExecInfoAtOrder& diff)
    {
        switch (diff.m_type)
        {
        case ORDER_BUY:
            for (const auto& ex: diff.m_exec) {
                m_spot_data[diff.m_code] += ex.m_number;
            }
            break;
        case ORDER_SELL:
            for (const auto& ex: diff.m_exec) {
                auto it = m_spot_data.find(diff.m_code);
                if (it == m_spot_data.end()) {
                    continue; // ���̂��ێ���񂪂Ȃ�(error)
                }
                if (it->second <= ex.m_number) {
                    m_spot_data.erase(it);
                } else {
                    it->second -= ex.m_number;
                }
            }
            break;
        default:
            break; // �s���Ȗ��(error)
        }
    }

    /*!
     *  @param  ������M�p�ۗL���ɔ��f
     *  @param  diff            �����(����) 1������
     *  @param  sv_rep_order    �����(����)�ƑΉ����锭���ςݐM�p�ԍϔ�������
     */
    void ReflectExecInfoToPosition(const StockExecInfoAtOrder diff, 
                                   const ServerRepLevOrder& sv_rep_order)
    {
        const bool b_sell = (diff.m_type == ORDER_SELL || diff.m_type == ORDER_REPSELL);
        switch (diff.m_type)
        {
        case ORDER_BUY:
        case ORDER_SELL:
            // �V�K����
            for (const auto& ex: diff.m_exec) {
                AddPosition(diff.m_code, ex.m_date, ex.m_value, b_sell, ex.m_number);
            }
            break;
        case ORDER_REPBUY:
        case ORDER_REPSELL:
            // �ԍϔ���
            {
                const auto itSv = sv_rep_order.find(diff.m_user_order_id);
                if (itSv == sv_rep_order.end()) {
                    return; // �Ȃ���������Ȃ�(error)
                }
                const StockTradingCommand& command(*itSv->second);
                garnet::YYMMDD bg_date(std::move(command.GetRepLevBargainDate()));
                if (bg_date.empty()) {
                    return; // �Ȃ����ԍϔ������߂���Ȃ�(error)
                }
                const float64 bg_value = command.GetRepLevBargainValue();
                for (const auto& ex: diff.m_exec) {
                    DecPosition(diff.m_code, bg_date, bg_value, b_sell, ex.m_number);
                }
            }
            break;
        default:
            break; // �s���Ȗ��(error)
        }
    }

    /*!
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     *  @param  number  ����
     */
    void InsertPosition(uint32_t code,
                        const garnet::YYMMDD& date, float64 value, bool b_sell,
                        int32_t number)
    {
        if (number <= 0) {
            return; // �����s��(error)
        }
        StockPosition pos(code, date, value, number, b_sell);
        m_position_data[code].emplace_back(IssuePositionID(), std::move(pos));
    }
    /*!
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     */
    void AddPosition(uint32_t code,
                     const garnet::YYMMDD& date, float64 value, bool b_sell,
                     int32_t number)
    {
        if (number <= 0) {
            return; // �����s��(error)
        }
        const auto itPosDat = m_position_data.find(code);
        if (itPosDat == m_position_data.end()) {
            InsertPosition(code, date, value, b_sell, number);
            return;
        }
        auto& pos_list(itPosDat->second);
        auto it = std::find_if(pos_list.begin(),
                               pos_list.end(),
                               [&date, value, b_sell]
                               (const std::pair<int32_t, StockPosition>& pos_unit) {
            const StockPosition& pos(pos_unit.second);
            if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                return false;
            }
            return pos.m_date == date && trade_utility::same_value(pos.m_value, value);
        });
        if (it == pos_list.end()) {
            InsertPosition(code, date, value, b_sell, number);
        } else {
            it->second.m_number += number;
        }
    }
    /*!
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     */
    void DecPosition(uint32_t code,
                     const garnet::YYMMDD& date, float64 value, bool b_sell,
                     int32_t number)
    {
        if (number <= 0) {
            return; // �����s��(error)
        }
        const auto itPosDat = m_position_data.find(code);
        if (itPosDat == m_position_data.end()) {
            return; // �Ȃ���������Ȃ�(error)
        }
        auto& pos_list(itPosDat->second);
        auto it = std::find_if(pos_list.begin(),
                               pos_list.end(),
                               [&date, value, b_sell]
                               (const std::pair<int32_t, StockPosition>& pos_unit) {
            const StockPosition& pos(pos_unit.second);
            if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                return false;
            }
            return pos.m_date == date && trade_utility::same_value(pos.m_value, value);
        });
        if (it == pos_list.end()) {
            return; // �Ȃ���������Ȃ�(error)
        } else {
            if (it->second.m_number > number) {
                it->second.m_number -= number;
            } else {
                // �ԍς��I�����̂ō폜
                pos_list.erase(it);
            }
        }
    }

public:
    PIMPL()
    : m_spot_data()
    , m_position_data()
    , m_position_id_source(0)
    , m_today_exec_order()
    {
    }

    /*!
     *  @brief  �����ۗL�����擾
     *  @param  code    �����R�[�h
     */
    int32_t GetSpotTradingStockNumber(const StockCode& code) const
    {
        const auto it = m_spot_data.find(code.GetCode());
        if (it == m_spot_data.end()) {
            return 0;
        }
        return it->second;
    }


    /*!
     *  @brief  ���ʃ`�F�b�NB
     *  @param  code    �����R�[�h
     *  @param  b_sell  �����t���O
     *  @param  number  �K�v����
     *  @return true    code/b_sell�̃|�W���Z�Ŋ�����number�ȏ゠��
     */
    bool CheckPosition(const StockCode& code, bool b_sell, int32_t number) const
    {
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            int32_t sum_number = 0;
            const auto& pos_list(itPosDat->second);
            for (const auto& pos_unit: pos_list) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                sum_number += pos.m_number;
                if (sum_number >= number) {
                    return true;
                }
            }
        }
        return false;
    }
    /*!
     *  @brief  ���ʃ`�F�b�NC
     *  @param  code    �����R�[�h
     *  @param  pos_id  ����ID�Q
     *  @return true    ���ʂ���ȏ�c���Ă�
     */
    bool CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id)
    {
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            const auto& pos_list(itPosDat->second);
            for (int32_t id: pos_id) {
                const auto itPos
                    = std::find_if(pos_list.begin(),
                                   pos_list.end(),
                                   [id](const std::pair<int32_t, StockPosition>& src) {
                    return src.first == id;
                });
                if (itPos != pos_list.end()) {
                    return true;
                }
            }
        }
        return false;
    }
    /*!
     *  @brief  �M�p�ۗL�����擾
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     */
    int32_t GetPositionNumber(const StockCode& code,
                              const garnet::YYMMDD& date, float64 value, bool b_sell) const
    {
        if (!date.empty()) {
            const auto itPosDat = m_position_data.find(code.GetCode());
            if (itPosDat != m_position_data.end()) {
                const auto& pos_list(itPosDat->second);
                for (const auto& pos_unit: pos_list) {
                    const StockPosition& pos(pos_unit.second);
                    if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                        continue;
                    }
                    if (pos.m_date == date && trade_utility::same_value(pos.m_value, value)) {
                        return pos.m_number;
                    }
                }
            }
        }
        return 0;
    }
    /*!
     *  @brief  �M�p�ۗL���擾
     *  @param  code    �����R�[�h
     *  @param  b_sell  �����t���O
     */
    std::vector<StockPosition> GetPosition(const StockCode& code, bool b_sell) const
    {
        std::vector<StockPosition> pos_vec;
        const auto itPosDat = m_position_data.find(code.GetCode());
        if (itPosDat != m_position_data.end()) {
            const auto& pos_list(itPosDat->second);
            pos_vec.reserve(pos_list.size());
            for (const auto& pos_unit: pos_list) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                pos_vec.emplace_back(pos);
            }
        }
        return pos_vec;
    }
    /*!
     *  @brief  �M�p�ۗL���ŗLID�𓾂�
     *  @param[in]  user_order_id   �����ԍ�(�\���p)
     *  @param[out] dst             �i�[��
     */
    void GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const
    {
        const auto it = m_today_exec_order.find(user_order_id);
        if (it == m_today_exec_order.end()) {
            return; // �Ȃ���������Ȃ�(error)
        }
        const StockExecInfoAtOrder& ex_info(it->second);
        const eOrderType odtype = ex_info.m_type;
        if (!ex_info.m_b_leverage) {
            return; // �����͑ΏۊO
        }
        if (odtype != ORDER_SELL && odtype != ORDER_BUY) {
            return; // �V�K�����̂ݑΏ�
        }
        const auto itPosDat = m_position_data.find(it->second.m_code);
        if (itPosDat == m_position_data.end()) {
            return; // �Ȃ����w������R�[�h�̌��ʂ��Ȃ�(error)
        }
        const bool b_sell = odtype == ORDER_SELL;
        for (const auto& ex: ex_info.m_exec) {
            const garnet::YYMMDD& date(ex.m_date);
            const float64 value = ex.m_value;
            for (const auto& pos_unit: itPosDat->second) {
                const StockPosition& pos(pos_unit.second);
                if ((b_sell && !pos.m_b_sell) || (!b_sell && pos.m_b_sell)) {
                    continue;
                }
                if (pos.m_date == date && trade_utility::same_value(pos.m_value, value)) {
                    dst.push_back(pos_unit.first);
                }
            }
        }
    }


    /*!
     *  @brief  �ۗL�����X�V
     *  @param  spot        �����ۗL��
     *  @param  position    �M�p�ۗL��
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position)
    {
        // �����͏㏑����OK
        m_spot_data.swap(std::move(SpotTradingStockData(spot)));
        // �M�p�͌ŗLID��t�����Ă���㏑��
        StockPositionData pos_data;
        for (const auto& pos: position) {
            const uint32_t code = pos.m_code.GetCode();
            const auto& cpos_list = m_position_data[code];
            const auto it = std::find_if(cpos_list.begin(),
                                         cpos_list.end(),
                                         [&pos](const std::pair<int32_t, StockPosition>& pos_unit)
            {
                return (pos == pos_unit.second);
            });
            // ���݂���|�W�Ȃ�ID�p���A�V�K�Ȃ�ID���s
            const int32_t pos_id = (it != cpos_list.end()) ?it->first
                                                           :IssuePositionID();
            pos_data[code].emplace_back(pos_id, pos);
        }
        m_position_data.swap(pos_data);
    }

    /*!
     *  @brief  ���������̍����𓾂�
     *  @param[in]  rcv_info    ��M���������(1�����t��)
     *  @param[out] diff_info   �O��Ƃ̍���(�i�[��)    
     */
    void GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                         std::vector<StockExecInfoAtOrder>& diff_info)
    {
        diff_info.reserve(rcv_info.size());
        for (const auto& rcv: rcv_info) {
            const auto it = m_today_exec_order.find(rcv.m_user_order_id);
            if (it == m_today_exec_order.end()) {
                // �����ԍ��P�ʂőO�񑶍݂��Ȃ�������܂邲�Ɠo�^
                diff_info.emplace_back(rcv);
            } else {
                // �O���葝���Ă��獷�������o�^
                const size_t prev_num = it->second.m_exec.size();
                const size_t now_num = rcv.m_exec.size();
                if (now_num > prev_num) {
                    StockExecInfoAtOrder t_ex(static_cast<const StockExecInfoAtOrderHeader&>(rcv));
                    t_ex.m_exec.reserve(now_num-prev_num);
                    for (size_t inx = prev_num; inx < now_num; inx++) {
                        t_ex.m_exec.emplace_back(it->second.m_exec[inx]);
                    }
                    diff_info.emplace_back(std::move(t_ex));
                }
            }
        }
    }
    /*!
     *  @brief  ���������X�V
     *  @param  rcv_info        ��M���������(1�����t��)
     *  @param  diff_info       �����(�O��Ƃ̍���)
     *  @param  sv_rep_order    �����(����)�ƑΉ����锭���ςݐM�p�ԍϔ�������
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                        const std::vector<StockExecInfoAtOrder>& diff_info,
                        const ServerRepLevOrder& sv_rep_order)
    {
        // ������ۗL�����ɔ��f
        for (const auto& diff: diff_info) {
            if (!diff.m_b_leverage) {
                // ����
                ReflectExecInfoToSpot(diff);
            } else {
                // �M�p
                ReflectExecInfoToPosition(diff, sv_rep_order);
            }
        }
        // ���������㏑��
        for (const auto& rcv: rcv_info) {
            m_today_exec_order.emplace(rcv.m_user_order_id, rcv);
        }
    }
};

StockHoldingsKeeper::StockHoldingsKeeper()
: m_pImpl(new PIMPL())
{
}

StockHoldingsKeeper::~StockHoldingsKeeper()
{
}

/*!
 *  @brief  �ۗL�����X�V
 *  @param  spot        �����ۗL��
 *  @param  position    �M�p�ۗL��(����)
 */
void StockHoldingsKeeper::UpdateHoldings(const SpotTradingsStockContainer& spot,
                                         const StockPositionContainer& position)
{
    m_pImpl->UpdateHoldings(spot, position);
}

/*!
 *  @brief  ���������̍����𓾂�
 *  @param[in]  rcv_info    ��M���������(1�����t��)
 *  @param[out] diff_info   �O��Ƃ̍���(�i�[��)    
 */
void StockHoldingsKeeper::GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                                          std::vector<StockExecInfoAtOrder>& diff_info)
{
    m_pImpl->GetExecInfoDiff(rcv_info, diff_info);
}
/*!
 *  @brief  ���������X�V
 *  @param  rcv_info        ��M���������(1�����t��)
 *  @param  diff_info       �����(�O��Ƃ̍���)
 *  @param  sv_rep_order    �����(����)�ƑΉ����锭���ςݐM�p�ԍϔ�������
 */
void StockHoldingsKeeper::UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                                         const std::vector<StockExecInfoAtOrder>& diff_info,
                                         const ServerRepLevOrder& sv_rep_order)
{
    m_pImpl->UpdateExecInfo(rcv_info, diff_info, sv_rep_order);
}

/*!
 *  @brief  �����`�F�b�N
 *  @param  code    �����R�[�h
 *  @param  number  �K�v����
 */
bool StockHoldingsKeeper::CheckSpotTradingStock(const StockCode& code, int32_t number) const
{
    const int32_t have_num = m_pImpl->GetSpotTradingStockNumber(code);
    return have_num >= number;
}
/*!
 *  @brief  �����ۗL�����擾
 *  @param  code    �����R�[�h
 */
int32_t StockHoldingsKeeper::GetSpotTradingStockNumber(const StockCode& code) const
{
    return m_pImpl->GetSpotTradingStockNumber(code);
}

/*!
 *  @brief  ���ʃ`�F�b�NA
 *  @param  code    �����R�[�h
 *  @param  date    ����
 *  @param  b_sell  �����t���O
 *  @param  value   ���P��
 *  @param  number  �K�v����
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code,
                                        const garnet::YYMMDD& date, float64 value, bool b_sell,
                                        int32_t number) const
{
    return m_pImpl->GetPositionNumber(code, date, value, b_sell) >= number;
}
/*!
 *  @brief  ���ʃ`�F�b�NB
 *  @param  code    �����R�[�h
 *  @param  b_sell  �����t���O
 *  @param  number  �K�v����
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code, bool b_sell, int32_t number) const
{
    return m_pImpl->CheckPosition(code, b_sell, number);
}
/*!
 *  @brief  ���ʃ`�F�b�NC
 *  @param  code    �����R�[�h
 *  @param  pos_id  ����ID�Q
 */
bool StockHoldingsKeeper::CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id)
{
    return m_pImpl->CheckPosition(code, pos_id);
}
/*!
 *  @brief  �M�p�ۗL�����擾
 *  @param  code    �����R�[�h
 *  @param  date    ����
 *  @param  value   ���P��
 *  @param  b_sell  �����t���O
 */
int32_t StockHoldingsKeeper::GetPositionNumber(const StockCode& code,
                                               const garnet::YYMMDD& date,
                                               float64 value,
                                               bool b_sell) const
{
    return m_pImpl->GetPositionNumber(code, date, value, b_sell);
}
/*!
 *  @brief  �M�p�ۗL���擾
 *  @param  code    �����R�[�h
 *  @param  b_sell  �����t���O
 */
std::vector<StockPosition> StockHoldingsKeeper::GetPosition(const StockCode& code,
                                                            bool b_sell) const
{
    return m_pImpl->GetPosition(code, b_sell);
}
/*!
 *  @brief  �M�p�ۗL���ŗLID�𓾂�
 *  @param[in]  user_order_id   �����ԍ�(�\���p)
 *  @param[out] dst             �i�[��
 */
void StockHoldingsKeeper::GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const
{
    return m_pImpl->GetPositionID(user_order_id, dst);
}

/*!
 *  @note   �M�p���ʔ�r
 *  @param  right
 *  @retval true    ��v
 */
bool StockPosition::operator==(const StockPosition& right) const
{
    // �����R�[�h�E�����E���P���E������ʂ܂ň�v������OK
    // (�ۗL���͌��Ȃ�)
    return (m_code.GetCode() == right.m_code.GetCode() &&
            m_date == right.m_date &&
            trade_utility::same_value(m_value, right.m_value) &&
            ((m_b_sell && right.m_b_sell) || (!m_b_sell && !right.m_b_sell)));
}

} // namespace trading
