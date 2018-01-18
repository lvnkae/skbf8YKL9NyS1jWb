/*!
 *  @file   trade_define.h
 *  @brief  �g���[�f�B���O�֘A��`
 *  @date   2017/05/05
 */
#pragma once

namespace trading
{

/*!
 *  @brief  ������
 */
enum eTradingType
{
    TYPE_NONE = 0,

    TYPE_STOCK,     //!< ����
    TYPE_FX,        //!< �ב�
    TYPE_CRYPTO_CR, //!< �Í��ʉ�(���z�ʉ�)
};

/*!
 *  @brief  �،���Ў��
 */
enum eSecuritiesType
{
    SEC_NONE = 0,

    SEC_SBI,        //!< SBI�،�
    SEC_KABUDOT,    //!< kabu.com
    SEC_MONEX,      //!< �}�l�b�N�X�،�
    SEC_NOMURA,     //!< �쑺暌�
};

/*!
 *  @brief  ����������
 */
enum eStockInvestmentsType
{
    INVESTMENTS_NONE = 0,   //!< �Ȃ�

    INVESTMENTS_TOKYO,      //!< ����
    INVESTMENTS_NAGOYA,     //!< ����
    INVESTMENTS_FUKUOKA,    //!< ����
    INVESTMENTS_SAPPORO,    //!< �D��
    INVESTMENTS_PTS,        //!< ���ݎ���V�X�e��
};

/*!
 *  @brief  �������
 */
enum eOrderType
{
    ORDER_NONE,

    ORDER_BUY,      //!< ����
    ORDER_SELL,     //!< ����
    ORDER_CORRECT,  //!< ����
    ORDER_CANCEL,   //!< ���
    ORDER_REPSELL,  //!< �Ԕ�
    ORDER_REPBUY,   //!< �Ԕ�

    NUM_ORDER,
};

/*!
 *  @brief  �t�������
 */
enum eReverseOrder
{
    RV_GTE, //!< �ȏ�(greater than or equal)
    RV_LTE, //!< �ȉ�(less than or equal)
};

/*!
 *  @brief  ��������
 */
enum eOrderCondition
{
    CONDITION_NONE = 0,

    CONDITION_OPENING,      //!< ���t��
    CONDITION_CLOSE,        //!< ����
    CONDITION_UNPROMOTED,   //!< �s��
};

} // namespace trading
