/*!
 *  @file   stock_trading_command.cpp
 *  @brief  ���������
 *  @date   2017/12/27
 */
#pragma once

#include "stock_trading_command.h"

#include "stock_code.h"
#include "stock_trading_tactics_utility.h"
#include "trade_struct.h"
#include "trade_utility.h"

namespace trading
{

/*!
 *  @brief  StockOrder�Ƀp�����[�^�����
 *  @param[in]  investments ��������
 *  @param[in]  code        �����R�[�h
 *  @param[in]  order_type  �������
 *  @param[in]  order_cond  ��������
 *  @param[in]  b_leverage  �M�p�t���O
 *  @param[in]  number      ��������
 *  @param[in]  value       �������i
 *  @param[out] o_order     �i�[��
 */
void SetOrderParam(eStockInvestmentsType investments,
                   const StockCode& code,
                   eOrderType order_type,
                   eOrderCondition order_cond,
                   bool b_leverage,
                   int32_t number,
                   float64 value,
                   StockOrder& o_order)
{
    o_order.m_code = code;
    o_order.m_number = number;
    o_order.m_value = value;
    o_order.m_b_leverage = b_leverage;
    o_order.m_b_market_order = trade_utility::is_market_order(value);
    o_order.m_type = order_type;
    o_order.m_condition = order_cond;
    o_order.m_investments = investments;
}

/*!
 *  @param  type            ���ߎ��
 *  @param  tactics_id      �헪ID
 *  @param  target_group    �ΏۃO���[�v<�헪�O���[�vID>
 */
StockTradingCommand::StockTradingCommand(eCommandType type,
                                         const StockCode& code,
                                         int32_t tactics_id)
: m_type(type)
, m_code(code.GetCode())
, m_tactics_id(tactics_id)
{
}

/*!
 *  @param  type            ���ߎ��
 *  @param  tactics_id      �헪ID
 *  @param  target_group    �ΏۃO���[�v<�헪�O���[�vID>
 */
StockTradingCommand_Order::StockTradingCommand_Order(eCommandType type,
                                                     const StockCode& code,
                                                     int32_t tactics_id,
                                                     int32_t group_id,
                                                     int32_t unique_id)
: StockTradingCommand(type, code, tactics_id)
, m_group_id(group_id)
, m_unique_id(unique_id)
, m_order()
{
}

/*!
 *  @brief  �R�}���h����(�ً}���[�h)
 *  @param  code            �����R�[�h
 *  @param  tactics_id      �헪ID
 *  @param  target_group    �ΏۃO���[�v<�헪�O���[�vID>
 */
StockTradingCommand_Emergency::StockTradingCommand_Emergency(const StockCode& code,
                                                             int32_t tactics_id,
                                                             const std::unordered_set<int32_t>& target_group)
: StockTradingCommand(EMERGENCY, code, tactics_id)
, m_target_group(target_group)
{
}

/*!
 *  @brief  �R�}���h�쐬(����[����])
 *  @param  investments ��������
 *  @param  type        ���ߎ��
 *  @param  code        �����R�[�h
 *  @param  tactics_id  �헪ID
 *  @param  group_id    �헪�O���[�vID
 *  @param  unique_id   �헪�����ŗLID
 *  @param  order_type  �������(eOrderType)
 *  @param  order_cond  ��������(eOrderCondition)
 *  @param  b_leverage  �M�p����t���O
 *  @param  number      ��������
 *  @param  value       �������i
 */
StockTradingCommand_BuySellOrder::StockTradingCommand_BuySellOrder(eStockInvestmentsType investments,
                                                                   const StockCode& code,
                                                                   int32_t tactics_id,
                                                                   int32_t group_id,
                                                                   int32_t unique_id,
                                                                   eOrderType order_type,
                                                                   eOrderCondition order_cond,
                                                                   bool b_leverage,
                                                                   int32_t number,
                                                                   float64 value)
: StockTradingCommand_Order(BUYSELL_ORDER, code, tactics_id, group_id, unique_id)
{
    SetOrderParam(investments, code, order_type, order_cond, b_leverage, number, value, m_order);
}

/*!
 *  @brief  �R�}���h�쐬(����[�M�p�ԍ�])
 *  @param  investments ��������
 *  @param  code        �����R�[�h
 *  @param  tactics_id  �헪ID
 *  @param  group_id    �헪�O���[�vID
 *  @param  unique_id   �헪�����ŗLID
 *  @param  order_type  �������(eOrderType)
 *  @param  order_cond  ��������(eOrderCondition)
 *  @param  number      ��������
 *  @param  value       �������i
 *  @param  bg_date     ����
 *  @param  bg_value    ���P��
 */
StockTradingCommand_RepLevOrder::StockTradingCommand_RepLevOrder(eStockInvestmentsType investments,
                                                                 const StockCode& code,
                                                                 int32_t tactics_id,
                                                                 int32_t group_id,
                                                                 int32_t unique_id,
                                                                 eOrderType order_type,
                                                                 eOrderCondition order_cond,
                                                                 int32_t number,
                                                                 float64 value,
                                                                 const garnet::YYMMDD& bg_date,
                                                                 float64 bg_value)
: StockTradingCommand_Order(REPAYMENT_LEV_ORDER, code, tactics_id, group_id, unique_id)
, m_bargain_date(bg_date)
, m_bargain_value(bg_value)
{
    SetOrderParam(investments, code, order_type, order_cond, true, number, value, m_order);
}

/*!
 *  @brief  �R�}���h�쐬(����[����/���])
 *  @param  src_command ��������(���̖��߂ŏ㏑��(����)�A�܂��͂��̖��߂������)
 *  @param  order_type  ���ߎ��
 *  @param  order_id    �����ԍ�(�،���Ђ����s��������)
 */
StockTradingCommand_ControllOrder::StockTradingCommand_ControllOrder(const StockTradingCommand& src_command,
                                                                     eOrderType order_type,
                                                                     int32_t order_id)
: StockTradingCommand_Order(CONTROL_ORDER,
                            src_command.GetCode(),
                            src_command.GetTacticsID(),
                            src_command.GetOrderGroupID(),
                            src_command.GetOrderUniqueID())
, m_order_id(order_id)
{
    m_order = std::move(src_command.GetOrder());
    m_order.m_type = order_type;
}

/*!
 *  @brief  �헪�O���[�vID�擾
 */
int32_t StockTradingCommand::GetOrderGroupID() const
{
    return tactics_utility::BlankGroupID();
}
/*!
 *  @brief  �헪�����ŗLID�擾
 */
int32_t StockTradingCommand::GetOrderUniqueID() const
{
    return tactics_utility::BlankOrderUniqueID();
}
/*!
 *  @brief  �������ԍ��擾
 */
int32_t StockTradingCommand::GetOrderID() const
{
    return trade_utility::BlankOrderID();
}

/*!
 *  @brief  �������̊��������߂�
 */
bool StockTradingCommand_Order::IsSameAttrOrder(const StockTradingCommand& right) const
{
    if (!right.IsOrder()) {
        return false;
    }
    return right.GetCode() == GetCode() &&
           right.GetTacticsID() == GetTacticsID() &&
           right.GetOrderGroupID() == m_group_id;
}
/*!
 *  @brief  right�͓����̔����������H
 *  @param  right   ��r���閽��
 */
bool StockTradingCommand_BuySellOrder::IsSameBuySellOrder(const StockTradingCommand& right) const
{
    if (right.GetType() != BUYSELL_ORDER) {
        return false;
    }
    const eOrderType order_type = m_order.m_type;
    const eOrderType r_order_type = right.GetOrderType();
    if (order_type != r_order_type) {
        return false;
    }
    return IsSameAttrOrder(right);
}

/*!
 *  @brief  number��������StockOrder��Copy
 */
void StockTradingCommand_Order::CopyStockOrderWithoutNumber(const StockTradingCommand& src)
{
    const int32_t org_number = m_order.m_number;
    m_order = std::move(src.GetOrder());
    m_order.m_number = org_number;
}
/*!
 *  @brief  StockOrder��number��������Copy
 */
void StockTradingCommand_BuySellOrder::CopyBuySellOrder(const StockTradingCommand& src)
{
    if (src.GetType() != BUYSELL_ORDER) {
        return;
    }
    // �������ߓ��m�ł̃R�s�[�Ɏg�����̂Ȃ̂�UniqueID��StockOrder(�����ȊO)�����ŗǂ�
    m_unique_id = src.GetOrderUniqueID();
    CopyStockOrderWithoutNumber(src);
}

} // namespace trading
