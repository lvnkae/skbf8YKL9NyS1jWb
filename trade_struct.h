/*!
 *  @file   trade_struct.h
 *  @brief  �g���[�f�B���O�֘A�\����
 *  @date   2017/12/21
 */
#pragma once

#include "trade_define.h"
#include "stock_code.h"

#include "hhmmss.h"
#include "yymmdd.h"

#include <string>
#include <vector>

namespace garnet { struct sTime; }

namespace trading
{

struct RcvResponseStockOrder;

/*!
 *  @brief  ������^�C���e�[�u��(��R�}��)
 */
struct StockTimeTableUnit
{
    enum eMode
    {
        CLOSED, //!< ��(�������Ȃ�)
        IDLE,   //!< ��MODE��������
        TOKYO,  //!< ���Z�،�������Ŕ���
        PTS,    //!< ���ݎ�����Ŕ���
    };

    garnet::HHMMSS m_hhmmss;//!< �����b
    eMode m_mode;           //!< ���[�h

    StockTimeTableUnit()
    : m_hhmmss()
    , m_mode(CLOSED)
    {
    }
    StockTimeTableUnit(const garnet::sTime& time)
    : m_hhmmss(time)
    , m_mode(CLOSED)
    {
    }

    /*!
     *  @brief  ���[�h������(�X�N���v�g�p)���������ʂɕϊ�
     *  @param  mode_str    �^�C���e�[�u�����[�h������
     */
    static eStockInvestmentsType ToInvestmentsTypeFromMode(eMode tt_mode)
    {
        if (tt_mode == StockTimeTableUnit::TOKYO) {
            return INVESTMENTS_TOKYO;
        } else if (tt_mode == StockTimeTableUnit::PTS) {
            return INVESTMENTS_PTS;
        } else {
            // ����/����/�D�؂͑Ή����Ȃ�
            return INVESTMENTS_NONE;
        }
    }

    /*!
     *  @brief  ���[�h�ݒ�
     *  @param  mode_str    �^�C���e�[�u�����[�h������
     *  @retval true    ����
     */
    bool SetMode(const std::string& mode_str);

    /*!
     *  @brief  ��r����
     *  @param  right   �E�Ӓl
     *  @retval true    �E�Ӓl���傫��
     */
    bool operator<(const StockTimeTableUnit& right) const {
        return m_hhmmss < right.m_hhmmss;
    }
};

/*!
 *  @brief  �������p�����[�^
 */
 struct StockOrder
{
    StockCode m_code;       //!< �����R�[�h
    int32_t m_number;       //!< ����
    float64 m_value;        //!< ���i
    bool m_b_leverage;      //!< �M�p�t���O
    bool m_b_market_order;  //!< ���s�t���O

    eOrderType m_type;                      //!< �������
    eOrderCondition m_condition;            //!< ����
    eStockInvestmentsType m_investments;    //!< �����

    StockOrder()
    : m_code()
    , m_number(0)
    , m_value(0.f)
    , m_b_leverage(false)
    , m_b_market_order(false)
    , m_type(ORDER_NONE)
    , m_condition(CONDITION_NONE)
    , m_investments(INVESTMENTS_NONE)
    {
    }

    /*!
     *  @param  rcv �����p�����[�^(��M�`��)
     */
    StockOrder(const RcvResponseStockOrder& rcv);

    /*!
     *  @brief  �����R�[�h�Q��
     */
    const StockCode& RefCode() const { return m_code; }
    /*!
     *  @brief  �����R�[�h�擾
     */
    uint32_t GetCode() const { return m_code.GetCode(); }
    /*!
     *  @brief  �����擾 
     */
    int32_t GetNumber() const { return m_number; }
    


    /*!
     *  @brief  �����̐���`�F�b�N
     *  @retval true    ����
     */
    bool IsValid() const
    {
        if (!m_code.IsValid()) {
            return false;   // �s�������R�[�h
        }
        if (m_number == 0) {
            return false;   // �����w��Ȃ�
        }
        if (m_b_market_order) {
            if (m_value > 0.0) {
                // ���s + ���i��(�X�N���v�g�ł͉��i�}�C�i�X=���s�w��)
                return false;
            }
            if (m_condition == CONDITION_UNPROMOTED) {
                // ���s�� + �s��
                return false;
            }
        } else {
            if (static_cast<int32_t>(m_value) < 1) {
                // �񐬍s�� + ���i0�~��ƕ����͂��肦�Ȃ�
                return false;
            }
        }
        if (m_b_leverage) {
            if (m_investments != INVESTMENTS_TOKYO) {
                // �M�p�͓��؈ȊO���肦�Ȃ�
                return false;
            }
        } else {
            if (m_investments != INVESTMENTS_TOKYO && m_investments != INVESTMENTS_PTS) {
                // ������s��(���w��܂��͖��Ή�)
                return false;
            }
        }
        if (m_type == ORDER_NONE) {
            // ������ʕs��
            return false;
        }
        //
        return true;
    }

    /*!
     *  @note   RcvResponseStockOrder�Ƃ̔�r
     */
    bool operator==(const RcvResponseStockOrder& right) const;
    bool operator!=(const RcvResponseStockOrder& right) const { return !(*this == right); }

    /*!
     *  @brief  ���b�Z�[�W�o�͗p�����񐶐�
     *  @param[in]  order_id    �����ԍ�
     *  @param[in]  name        ������
     *  @param[in]  number      ����
     *  @param[in]  value       ���i
     *  @param[out] o_str       �i�[��
     *  @note   twitter�o�͗p
     */
    void BuildMessageString(int32_t order_id,
                            const std::wstring& name,
                            int32_t number,
                            float64 value,
                            std::wstring& o_str) const;
};

/*!
 *  @brief  ����������[��M�`��]
 */
 struct RcvResponseStockOrder
{
    int32_t m_order_id;                 //!< �����ԍ� �Ǘ��p(SBI:�O���[�o�����ۂ�/)
    int32_t m_user_order_id;            //!< �����ԍ� �\���p(SBI:���[�U�ŗL/)
    eOrderType m_type;                  //!< �������
    eStockInvestmentsType m_investments;//!< ��������
    uint32_t m_code;                    //!< �����R�[�h
    int32_t m_number;                   //!< ��������
    float64 m_value;                    //!< �������i
    bool m_b_leverage;                  //!< �M�p�t���O

    RcvResponseStockOrder();
};

/*!
 *  @brief  �������(1��蕪)
 */
struct StockExecInfo
{
    int32_t m_number;           //!< ��芔��
    float64 m_value;            //!< ���P��
    garnet::YYMMDD m_date;      //!< ���N����
    garnet::HHMMSS m_time;      //!< ��莞��

    StockExecInfo()
    : m_number(0)
    , m_value(0.0)
    , m_date()
    , m_time()
    {
    }

    StockExecInfo(const garnet::sTime& datetime,
                  int32_t number,
                  float64 value)
    : m_number(number)
    , m_value(value)
    , m_date(datetime)
    , m_time(datetime)
    {
    }
};

/*!
 *  @brief  �������(1������)�w�b�_
 */
struct StockExecInfoAtOrderHeader
{
    int32_t m_user_order_id;            //!< �����ԍ� �\���p(SBI:���[�U�ŗL/)
    eOrderType  m_type;                 //!< �������
    eStockInvestmentsType m_investments;//!< �������
    uint32_t m_code;                    //!< �����R�[�h
    bool m_b_leverage;                  //!< �M�p�t���O
    bool m_b_complete;                  //!< ��芮���t���O

    StockExecInfoAtOrderHeader();
};
/*!
 *  @brief  �������(1������)
 */
struct StockExecInfoAtOrder : public StockExecInfoAtOrderHeader
{
    std::vector<StockExecInfo>  m_exec; //!< �����

    /*!
     */
    StockExecInfoAtOrder()
    : StockExecInfoAtOrderHeader()
    , m_exec()
    {
    }
    /*!
     *  @brief  �w�b�_�����������p���R���X�g���N�^
     */
    StockExecInfoAtOrder(const StockExecInfoAtOrderHeader& src)
    : StockExecInfoAtOrderHeader(src)
    , m_exec()
    {
    }
};

} // namespace trading
