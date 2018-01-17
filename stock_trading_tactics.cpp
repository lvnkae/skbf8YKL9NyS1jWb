/*!
 *  @file   stock_trading_tactics.cpp
 *  @brief  ������헪
 *  @date   2017/12/08
 */
#include "stock_trading_tactics.h"

#include "stock_portfolio.h"
#include "stock_trading_command.h"
#include "trade_assistant_setting.h"
#include "trade_define.h"

#include <algorithm>

namespace trading
{

/*!
 */
StockTradingTactics::StockTradingTactics()
: m_emergency()
, m_fresh()
, m_repayment()
{
}


/*!
 *  @brief  �ً}���[�h��ǉ�����
 *  @param  emergency   �ً}���[�h�ݒ�
 */
void StockTradingTactics::AddEmergencyMode(const Emergency& emergency)
{
    m_emergency.emplace_back(emergency);
}
/*!
 *  @brief  �V�K������ǉ�
 *  @param  order   �����ݒ�
 */
void StockTradingTactics::AddFreshOrder(const Order& order)
{
    m_fresh.emplace_back(order);
}
/*!
 *  @brief  �ԍϒ�����ǉ�
 *  @param  order   �����ݒ�
 */
void StockTradingTactics::AddRepaymentOrder(const RepOrder& order)
{
    m_repayment.emplace_back(order);
}



/*!
 *  @brief  ����
 *  @param  now_time    ���ݎ����b
 *  @param  sec_time    ���Z�N�V�����J�n����
 *  @param  valuedata   ���i�f�[�^(1������)
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 */
bool StockTradingTactics::Trigger::Judge(const garnet::HHMMSS& now_time,
                                         const garnet::HHMMSS& sec_time,
                                         const StockValueData& valuedata,
                                         TradeAssistantSetting& script_mng) const
{
    if (valuedata.m_value_data.empty()) {
        return false; // ���i�f�[�^���Ȃ������画�肵�Ȃ�
    }

    switch (m_type)
    {
    case VALUE_GAP:
        {
            if (static_cast<int32_t>(valuedata.m_close) <= 0) {
                // 0���Z�΍�(error)
                // ��ꏉ�������͑O���I�l�����݂��Ȃ�(���唼�̓G���[)
                return false;
            }   
            float64 v_high = 0.f;   // ���ԍ��l
            float64 v_low = 0.f;    // ���Ԉ��l
            float64 v_open = 0.f;   // ���Ԏn�l
            // �w�����[����,�w��b�O]�̎n�l/���l/���l�𓾂�
            {
                const int32_t pastsec = now_time.GetPastSecond();
                const int32_t rangesec = m_signed_param;
                const size_t presz = valuedata.m_value_data.size();
                //auto rit_end = valuedata.m_value_data.rend();
                int32_t rinx = 0;
                for (auto rit = valuedata.m_value_data.rbegin(); rit != valuedata.m_value_data.rend(); ++rit) {
                    if ((pastsec - rit->m_hhmmss.GetPastSecond()) <= rangesec) {
                        // �o�����Ȃ�(=�f�[�^�擪)�Ȃ�O���I�l�����l�Ƃ���
                        const float64 v_now = (rit->m_volume == 0) ?valuedata.m_close
                                                                   :rit->m_value;
                        v_high = std::max(v_high, v_now);
                        v_low  = (static_cast<int32_t>(v_low) == 0) ?v_now
                                                                    :std::min(v_low, v_now);
                        v_open = v_now;
                    }
                    rinx++;
                }
            }
            // ���Ԏn�l�����݂���ꍇ�̂݃M���b�v�𒲂ׂ�
            if (v_open > 0.f) {
                if (m_float_param > 0.f) {
                    const float64 rateU = ((v_high-v_open)/v_open)*100.f;
                    return rateU >= m_float_param; 
                } else {
                    const float64 rateD = ((v_low-v_open)/v_open)*100.f;
                    return rateD <= m_float_param;
                }
            }
        }
        break;

    case NO_CONTRACT:
        {
            const int32_t pastsec = now_time.GetPastSecond();
            const int32_t sectsec = sec_time.GetPastSecond();
            const int32_t latestsec = valuedata.m_value_data.back().m_hhmmss.GetPastSecond();
            const int32_t diffsec = pastsec - std::max(sectsec, latestsec);
            return diffsec >= m_signed_param;
        }
        break;

    case SCRIPT_FUNCTION:
        {
            const auto& latest = valuedata.m_value_data.back();
            // �o�����Ȃ��Ȃ画�肵�Ȃ�(�f�[�^�擪�̂ݎ��ԋL�^�p�ɑ��݂�����)
            if (latest.m_volume > 0) {
                float64 lvalue = latest.m_value;
                return script_mng.CallJudgeFunction(m_signed_param,
                                                    lvalue,
                                                    valuedata.m_high,
                                                    valuedata.m_low,
                                                    valuedata.m_close);
            }
        }
    }

    return false;
}

/*!
 *  @brief  �헪����
 *  @param  investments     ���ݎ�������
 *  @param  now_time        ���ݎ����b
 *  @param  sec_time        ���Z�N�V�����J�n����
 *  @param  em_group        �ً}���[�h�ΏۃO���[�v<�헪�O���[�vID>
 *  @param  valuedata       ���i�f�[�^(1������)
 *  @param  script_mng      �O���ݒ�(�X�N���v�g)�Ǘ���
 *  @param  enqueue_func    ���߂��L���[�ɓ����֐�
 */
void StockTradingTactics::Interpret(eStockInvestmentsType investments,
                                    const garnet::HHMMSS& now_time,
                                    const garnet::HHMMSS& sec_time,
                                    const std::unordered_set<int32_t>& em_group,
                                    const StockValueData& valuedata,
                                    TradeAssistantSetting& script_mng,
                                    const EnqueueFunc& enqueue_func) const
{
    const StockCode& s_code(valuedata.m_code);
    const bool b_pts = investments == INVESTMENTS_PTS;

    // �ً}���[�h����
    for (const auto& emg: m_emergency) {
        if (!emg.Judge(now_time, sec_time, valuedata, script_mng)) {
            continue;
        }
        std::shared_ptr<StockTradingCommand> command_ptr(
            new StockTradingCommand_Emergency(s_code,
                                              m_unique_id,
                                              emg.RefTargetGroup()));
        enqueue_func(command_ptr);
    }
    // �V�K��������
    for (const auto& order: m_fresh) {
        if (em_group.end() != em_group.find(order.GetGroupID())) {
            continue; // �ً}���[�h������
        }
        if (b_pts && order.GetIsLeverage()) {
            continue; // PTS���͐M�p�s��
        }
        if (!order.Judge(now_time, sec_time, valuedata, script_mng)) {
            continue;
        }
        const auto& latest = valuedata.m_value_data.back();
        const float64 value = script_mng.CallGetValueFunction(order.GetValueFuncReference(),
                                                              latest.m_value,
                                                              valuedata.m_high,
                                                              valuedata.m_low,
                                                              valuedata.m_close);
        const trading::eOrderType otype = (order.GetType() == BUY) ?ORDER_BUY 
                                                                   :ORDER_SELL;
        std::shared_ptr<StockTradingCommand> command_ptr(
            new StockTradingCommand_BuySellOrder(investments,
                                                 s_code,
                                                 m_unique_id,
                                                 order.GetGroupID(),
                                                 order.GetUniqueID(),
                                                 otype,
                                                 CONDITION_NONE, // >ToDo< �����Ή�
                                                 order.GetIsLeverage(),
                                                 order.GetNumber(),
                                                 value));
        enqueue_func(command_ptr);
    }
    // �ԍϒ�������
    for (const auto& order: m_repayment) {
        if (em_group.end() != em_group.find(order.GetGroupID())) {
            continue; // �ً}���[�h������
        }
        if (b_pts && order.GetIsLeverage()) {
            continue; // PTS���͐M�p�s��
        }
        if (!order.Judge(now_time, sec_time, valuedata, script_mng)) {
            continue;
        }
        const auto& latest = valuedata.m_value_data.back();
        const float64 value = script_mng.CallGetValueFunction(order.GetValueFuncReference(),
                                                              latest.m_value,
                                                              valuedata.m_high,
                                                              valuedata.m_low,
                                                              valuedata.m_close);
        std::shared_ptr<StockTradingCommand> command_ptr;
        if (!order.GetIsLeverage()) {
            // ������
            command_ptr.reset(new StockTradingCommand_BuySellOrder(investments,
                                                                   s_code,
                                                                   m_unique_id,
                                                                   order.GetGroupID(),
                                                                   order.GetUniqueID(),
                                                                   ORDER_SELL,
                                                                   CONDITION_NONE, // >ToDo< �����Ή�
                                                                   order.GetIsLeverage(),
                                                                   order.GetNumber(),
                                                                   value));
        } else {
            // �M�p�ԍϔ���
            const trading::eOrderType otype = (order.GetType() == BUY) ?ORDER_REPBUY
                                                                       :ORDER_REPSELL;
            command_ptr.reset(new StockTradingCommand_RepLevOrder(investments,
                                                                  s_code,
                                                                  m_unique_id,
                                                                  order.GetGroupID(),
                                                                  order.GetUniqueID(),
                                                                  otype,
                                                                  CONDITION_NONE, // >ToDo< �����Ή�
                                                                  order.GetNumber(),
                                                                  value,
                                                                  order.GetBargainDate(),
                                                                  order.GetBargainValue()));
        }
        enqueue_func(command_ptr);
    }
}

} // namespace trading
