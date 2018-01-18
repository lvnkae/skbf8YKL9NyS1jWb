/*!
 *  @file   stock_trading_command.h
 *  @brief  ���������
 *  @date   2017/12/27
 */
#pragma once

#include "trade_struct.h"

#include "yymmdd.h"

#include <unordered_set>
#include <vector>

namespace trading
{

/*!
 *  @brief  ���������
 *  @note   �p���p
 *  @note   ����헪���甭���Ǘ��ւ̎w���Ɏg��
 */
class StockTradingCommand
{
public:
    enum eCommandType
    {
        NONE = 0,

        EMERGENCY,          //!< �ً}���[�h(�ʏ풍���S����&�ʏ픭���ꎞ��~)
        BUYSELL_ORDER,      //!< ��������(���������E�M�p�V�K����)
        REPAYMENT_LEV_ORDER,//!< �M�p�ԍϒ���(�M�p�ԍϔ���)
        CONTROL_ORDER,      //!< ���䒍��(���i�����E�������)
    };

    /*!
     *  @brief  ���ߎ�ʂ𓾂�
     */
    eCommandType GetType() const { return m_type; }
    /*!
     *  @brief  �����R�[�h�𓾂�
     */
    uint32_t GetCode() const { return m_code; }
    /*!
     *  @brief  �헪ID�𓾂�
     */
    int32_t GetTacticsID() const { return m_tactics_id; }

    /*!
     *  @brief  ���������߂�
     */
    virtual bool IsOrder() const { return false; }

    /*!
     *  @brief  �ً}���[�h�ΏۃO���[�v�𓾂�(�R�s�[)
     */
    virtual std::unordered_set<int32_t> GetEmergencyTargetGroup() const { return std::unordered_set<int32_t>(); }

    /*!
     *  @brief  �������p�����[�^�𓾂�(�R�s�[)
     */
    virtual StockOrder GetOrder() const { return StockOrder(); }
    /*!
     *  @brief  �헪�O���[�vID�擾
     */
    virtual int32_t GetOrderGroupID() const;
    /*!
     *  @brief  �헪�����ŗLID�擾
     */
    virtual int32_t GetOrderUniqueID() const;
    /*!
     *  @brief  ��������ʎ擾
     */
    virtual eOrderType GetOrderType() const{ return ORDER_NONE; }
    /*!
     *  @brief  �����������擾
     */
    virtual int32_t GetOrderNumber() const { return 0; }
    /*!
     *  @brief  ���M�p������
     */
    virtual bool IsLeverageOrder() const { return false; }
    /*!
     *  @brief  �������ԍ��擾
     */
    virtual int32_t GetOrderID() const;
    /*!
     *  @brief  �ԍϒ����F�����w��𓾂�
     */
    virtual garnet::YYMMDD GetRepLevBargainDate() const { return garnet::YYMMDD(); }
    /*!
     *  @brief  �ԍϒ����F���P���w��𓾂�
     */
    virtual float64 GetRepLevBargainValue() const { return 0.0; }

    /*!
     *  @brief  �������̊��������߂�
     */
    virtual bool IsSameAttrOrder(const StockTradingCommand& right) const { return false; }
    /*!
     *  @brief  right�͓����̔����������H
     *  @param  right   ��r���閽��
     */
    virtual bool IsSameBuySellOrder(const StockTradingCommand& right) const { return false; }

    /*!
     *  @brief  �����������ݒ�
     */
    virtual void SetOrderNumber(int32_t number) {}
    /*!
     *  @brief  ���������i�ݒ�
     */
    virtual void SetOrderValue(float64 value) {}
    /*!
     *  @brief  BuySellOrder���m�̃R�s�[
     *  @param  src �R�s�[��
     */
    virtual void CopyBuySellOrder(const StockTradingCommand& src) {}
    /*!
     *  @brief  �ԍϒ����F����/���P���ݒ�
     *  @param  date    ����
     *  @param  value   ���P��
     */
    virtual void SetRepLevBargain(const garnet::YYMMDD& date, float64 value) {}

    virtual ~StockTradingCommand() {};

protected:
    /*!
     *  @param  type            ���ߎ��
     *  @param  code            �����R�[�h
     *  @param  tactics_id      �헪ID
     */
    StockTradingCommand(eCommandType type, const StockCode& code, int32_t tactics_id);

private:
    eCommandType m_type;    //!< ���ߎ��
    uint32_t m_code;        //!< �����R�[�h
    int32_t m_tactics_id;   //!< �����헪ID

    StockTradingCommand(const StockTradingCommand&);
    StockTradingCommand(StockTradingCommand&&);
    StockTradingCommand& operator= (const StockTradingCommand&);
};

/*!
 *  @brief  ����������
 *  @note   �p���p
 */
class StockTradingCommand_Order : public StockTradingCommand
{
private:
    bool IsOrder() const override { return true; }
    /*!
     *  @brief  �������p�����[�^�𓾂�(�R�s�[)
     */
    StockOrder GetOrder() const override { return m_order; }
    /*!
     *  @brief  �헪�O���[�vID�擾
     */
    int32_t GetOrderGroupID() const override { return m_group_id; }
    /*!
     *  @brief  ��������ʎ擾
     */
    eOrderType GetOrderType() const override { return m_order.m_type; }
    /*!
     *  @brief  �����������擾
     */
    int32_t GetOrderNumber() const override { return m_order.m_number; }
    /*!
     *  @brief  ���M�p������
     */
    bool IsLeverageOrder() const override { return m_order.m_b_leverage; }

    /*!
     *  @brief  �����������ݒ�
     */
    void SetOrderNumber(int32_t number) override { m_order.m_number = number; }
    /*!
     *  @brief  ���������i�ݒ�
     */
    void SetOrderValue(float64 value) override { m_order.m_value = value; }

protected:
    /*!
     *  @brief  �헪�����ŗLID�擾
     */
    int32_t GetOrderUniqueID() const override { return m_unique_id; }
    /*!
     *  @brief  �������̊��������߂�
     */
    bool IsSameAttrOrder(const StockTradingCommand& right) const override;
    /*!
     *  @brief  number��������StockOrder���R�s�[
     *  @param  src �R�s�[��
     */
    void CopyStockOrderWithoutNumber(const StockTradingCommand& src);

    /*!
     *  @param  type        ���ߎ��
     *  @param  code        �����R�[�h
     *  @param  tactics_id  �헪ID
     *  @param  group_id    �헪�O���[�vID
     *  @param  unique_id   �헪�����ŗLID
     */
    StockTradingCommand_Order(eCommandType type,
                              const StockCode& code,
                              int32_t tactics_id,
                              int32_t group_id,
                              int32_t unique_id);

    int32_t m_group_id;     //!< �헪�O���[�vID
    int32_t m_unique_id;    //!< �헪�����ŗLID
    StockOrder m_order;     //!< �������p�����[�^
};

/*!
 *  @brief  �ً}���[�h����
 */
class StockTradingCommand_Emergency : public StockTradingCommand
{
public:
    /*!
     *  @brief  �R�}���h����(�ً}���[�h)
     *  @param  code            �����R�[�h
     *  @param  tactics_id      �헪ID
     *  @param  target_group    �ΏۃO���[�v<�헪�O���[�vID>
     */
    StockTradingCommand_Emergency(const StockCode& code,
                                  int32_t tactics_id,
                                  const std::unordered_set<int32_t>& target_group);

private:
    /*!
     *  @brief  �ً}���[�h�ΏۃO���[�v�𓾂�(�R�s�[)
     */
    std::unordered_set<int32_t> GetEmergencyTargetGroup() const override { return m_target_group; }

    //! �ΏۃO���[�v
    std::unordered_set<int32_t> m_target_group;
};

/*!
 *  @brief  ��������������
 *  @note   ���������E�M�p�V�K����
 */
class StockTradingCommand_BuySellOrder : public StockTradingCommand_Order
{
public:
    /*!
     *  @brief  �R�}���h�쐬(����[����])
     *  @param  investments ��������
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
    StockTradingCommand_BuySellOrder(eStockInvestmentsType investments,
                                     const StockCode& code,
                                     int32_t tactics_id,
                                     int32_t group_id,
                                     int32_t unique_id,
                                     eOrderType order_type,
                                     eOrderCondition order_cond,
                                     bool b_leverage,
                                     int32_t number,
                                     float64 value);

private:
    /*!
     *  @brief  right�͓����̔����������H
     *  @param  right   ��r���閽��
     */
    bool IsSameBuySellOrder(const StockTradingCommand& right) const override;
    /*!
     *  @brief  BuySellOrder���m�̃R�s�[
     *  @param  src �R�s�[��
     */
    void CopyBuySellOrder(const StockTradingCommand& src) override;
};

/*!
 *  @brief  �M�p�ԍϒ���
 *  @note   �M�p�ԍϔ���
 */
class StockTradingCommand_RepLevOrder : public StockTradingCommand_Order
{
public:
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
    StockTradingCommand_RepLevOrder(eStockInvestmentsType investments,
                                    const StockCode& code,
                                    int32_t tactics_id,
                                    int32_t group_id,
                                    int32_t unique_id,
                                    eOrderType order_type,
                                    eOrderCondition order_cond,
                                    int32_t number,
                                    float64 value,
                                    const garnet::YYMMDD& bg_date,
                                    float64 bg_value);

private:
    /*!
     *  @brief  �ԍϒ����F�����w��𓾂�
     */
    garnet::YYMMDD GetRepLevBargainDate() const override { return m_bargain_date; }
    /*!
     *  @brief  �ԍϒ����F���P���w��𓾂�
     */
    float64 GetRepLevBargainValue() const override { return m_bargain_value; }
    
    /*!
     *  @brief  �ԍϒ����F����/���P���ݒ�
     *  @param  date    ����
     *  @param  value   ���P��
     */
    void SetRepLevBargain(const garnet::YYMMDD& date, float64 value) override
    {
        m_bargain_date = date;
        m_bargain_value = value;
    }

    garnet::YYMMDD m_bargain_date;  //!< ����
    float64 m_bargain_value;    //!< ���P��
};

/*!
 *  @brief  ���䒍��
 *  @note   ���i�����E�������
 */
class StockTradingCommand_ControllOrder : public StockTradingCommand_Order
{
public:
    /*!
     *  @brief  �R�}���h�쐬(����[����/���])
     *  @param  src_command ��������(���̖��߂ŏ㏑��(����)�A�܂��͂��̖��߂������)
     *  @param  order_type  ���ߎ��
     *  @param  order_id    �����ԍ�(�،���Ђ����s��������/�Ǘ��p)
     */
    StockTradingCommand_ControllOrder(const StockTradingCommand& src_command,
                                      eOrderType order_type,
                                      int32_t order_id);

private:
    /*!
     *  @brief  �������ԍ��擾
     */
    int32_t GetOrderID() const override { return m_order_id; }

    int32_t m_order_id; //!< �����ԍ�(�،���Ђ����s��������/�Ǘ��p)
};

} // namespace trading
