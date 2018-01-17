/*!
 *  @file   stock_trading_tactics.h
 *  @brief  ������헪
 *  @date   2017/12/08
 */
#pragma once

#include "stock_trading_command_fwd.h"
#include "trade_define.h"

#include "yymmdd.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace garnet { struct HHMMSS; }

namespace trading
{

struct StockValueData;
class StockTradingCommand;
class TradeAssistantSetting;

/*!
*  @brief  ������헪
*/
class StockTradingTactics
{
private:
    enum eTriggerType
    {
        TRRIGER_NONE,

        VALUE_GAP,      // �}�ϓ�(����������t�ȓ��Ɋ���r�ω�(r>0:�㏸/r<0:����)
        NO_CONTRACT,    // �����Ԋu(����t�ȏ��肪�Ȃ�����)
        SCRIPT_FUNCTION,// �X�N���v�g�֐�����
    };
    enum eTacticsOrder
    {
        ORDER_NONE,

        BUY,    // ��������
        SELL,   // ���蒍��
    };

public:
    /*!
     *  @brief  �g���K�[�ݒ�
     *  @note   ���^�����N�����^�C�~���O
     */
    class Trigger
    {
    private:
        eTriggerType m_type;       //!< �^�C�v
        float32 m_float_param;     //!< �t���[�p�����[�^(32bit���������_)
        int32_t m_signed_param;    //!< �t���[�p�����[�^(32bit�����t��)
    public:
        Trigger()
        : m_type(eTriggerType::TRRIGER_NONE)
        , m_float_param(0.f)
        , m_signed_param(0)
        {
        }

        void Set_ValueGap(float32 persent, int32_t sec)
        {
            m_type = VALUE_GAP;
            m_float_param = persent;
            m_signed_param = sec;
        }
        void Set_NoContract(int32_t sec)
        {
            m_type = NO_CONTRACT;
            m_signed_param = sec;
        }
        void Set_ScriptFunction(int32_t func_ref)
        {
            m_type = SCRIPT_FUNCTION;
            m_signed_param =func_ref;
        }

        void Copy(const Trigger& src)
        {
            *this = src;
        }

        bool empty() const { return m_type == TRRIGER_NONE; }

        /*!
         *  @brief  ����
         *  @param  now_time    ���ݎ����b
         *  @param  sec_time    ���Z�N�V�����J�n����
         *  @param  valuedata   ���i�f�[�^(1������)
         *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
         *  @retval true        �g���K�[����
         */
        bool Judge(const garnet::HHMMSS& now_time,
                   const garnet::HHMMSS& sec_time,
                   const StockValueData& valuedata,
                   TradeAssistantSetting& script_mng) const;
    };

    /*!
     *  @brief  �ً}���[�h�ݒ�
     */
    class Emergency : public Trigger
    {
    private:
        std::unordered_set<int32_t> m_group;    //!< �ΏۃO���[�v�ԍ�

    public:
        Emergency()
        : Trigger()
        , m_group()
        {
        }

        void AddTargetGroupID(int32_t group_id) { m_group.insert(group_id); }
        void SetCondition(const Trigger& trigger) { Copy(trigger); }
        const std::unordered_set<int32_t>& RefTargetGroup() const { return m_group; }
    };

    /*!
     *  @brief  �����ݒ�
     */
    class Order : public Trigger
    {
    private:
        int32_t m_unique_id;    //!< �����ŗLID
        int32_t m_group_id;     //!< �헪�O���[�vID(����O���[�v�̒����͔r�����䂳���)
        eTacticsOrder m_type;   //!< �^�C�v
        int32_t m_value_func;   //!< ���i�擾�֐�(���t�@�����X)
        int32_t m_number;       //!< ����
        bool m_b_leverage;      //!< �M�p�t���O

        void SetParam(eTacticsOrder type, bool b_leverage, int32_t func_ref, int32_t number)
        {
            m_type = type;
            m_value_func = func_ref;
            m_number = number;
            m_b_leverage = b_leverage;
        }

    public:
        Order()
        : Trigger()
        , m_unique_id(0)
        , m_group_id(0)
        , m_type(eTacticsOrder::ORDER_NONE)
        , m_number(0)
        , m_value_func(0)
        , m_b_leverage(false)
        {
        }

        void SetUniqueID(int32_t unique_id) { m_unique_id = unique_id; }
        void SetGroupID(int32_t group_id) { m_group_id = group_id; }
        void SetBuy(bool b_leverage, int32_t func_ref, int32_t number) { SetParam(BUY, b_leverage, func_ref, number); }
        void SetSell(bool b_leverage, int32_t func_ref, int32_t number) { SetParam(SELL, b_leverage, func_ref, number); }
        void SetCondition(const Trigger& trigger) { Copy(trigger); }

        eTacticsOrder GetType() const { return m_type; }
        int32_t GetGroupID() const { return m_group_id; }
        int32_t GetUniqueID() const { return m_unique_id; }
        int32_t GetNumber()  const { return m_number; }
        bool GetIsLeverage() const { return m_b_leverage; }

        /*!
         *  @brief  ���i�擾�֐��Q�Ǝ擾
         */
        int32_t GetValueFuncReference() const { return m_value_func; }
    };

    /*!
     *  @brief  �ԍϒ����ݒ�
     */
    class RepOrder : public Order
    {
    private:
        garnet::YYMMDD m_bargain_date;  //! ����
        float64 m_bargain_value;        //! ���P��
    public:
        RepOrder()
        : Order()
        , m_bargain_date()
        , m_bargain_value(0.0)
        {
        }

        void SetBargainInfo(const std::string& date_str, float64 value)
        {
            m_bargain_date = std::move(garnet::YYMMDD::Create(date_str));
            m_bargain_value = value;
        }

        garnet::YYMMDD GetBargainDate() const { return m_bargain_date; }
        float64 GetBargainValue() const { return m_bargain_value; }
    };

    /*!
     */
    StockTradingTactics();

    /*!
     *  @brief  �ŗLID�𓾂�
     */
    int32_t GetUniqueID() const { return m_unique_id; }

    /*!
     *  @brief  �ŗLID���Z�b�g����
     *  @note   unique�ł��邩�̓Z�b�g���鑤���ۏ؂��邱��
     */
    void SetUniqueID(int32_t id) { m_unique_id = id; }
    /*!
     *  @brief  �ً}���[�h��ǉ�����
     *  @param  emergency   �ً}���[�h�ݒ�
     */
    void AddEmergencyMode(const Emergency& emergency);
    /*!
     *  @brief  �V�K������ǉ�
     *  @param  order   �����ݒ�
     */
    void AddFreshOrder(const Order& order);
    /*!
     *  @brief  �ԍϒ�����ǉ�
     *  @param  order   �����ݒ�
     */
    void AddRepaymentOrder(const RepOrder& order);

    /*!
     */
    typedef std::function<void(const StockTradingCommandPtr&)> EnqueueFunc;

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
    void Interpret(eStockInvestmentsType investments,
                   const garnet::HHMMSS& now_time,
                   const garnet::HHMMSS& sec_time,
                   const std::unordered_set<int32_t>& em_group,
                   const StockValueData& valuedata,
                   TradeAssistantSetting& script_mng,
                   const EnqueueFunc& enqueue_func) const;

private:
    int32_t m_unique_id;                //!< �ŗLID(�헪�f�[�^�ԂŔ��Ȃ�����)
    std::vector<Emergency> m_emergency; //!< �ً}���[�h���X�g
    std::vector<Order> m_fresh;         //!< �V�K�������X�g
    std::vector<RepOrder> m_repayment;  //!< �ԍϒ������X�g
};

} // trading
