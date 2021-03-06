/*!
 *  @file   trade_define.h
 *  @brief  g[fBOÖAè`
 *  @date   2017/05/05
 */
#pragma once

namespace trading
{

/*!
 *  @brief  æøíÊ
 */
enum eTradingType
{
    TYPE_NONE = 0,

    TYPE_STOCK,     //!< ®
    TYPE_FX,        //!< ×Ö
    TYPE_CRYPTO_CR, //!< ÃÊÝ(¼zÊÝ)
};

/*!
 *  @brief  ØïÐíÊ
 */
enum eSecuritiesType
{
    SEC_NONE = 0,

    SEC_SBI,        //!< SBIØ
    SEC_KABUDOT,    //!< kabu.com
    SEC_MONEX,      //!< }lbNXØ
    SEC_NOMURA,     //!< ìºæ
};

/*!
 *  @brief  æøíÊ
 */
enum eStockInvestmentsType
{
    INVESTMENTS_NONE = 0,   //!< Èµ

    INVESTMENTS_TOKYO,      //!< Ø
    INVESTMENTS_NAGOYA,     //!< ¼Ø
    INVESTMENTS_FUKUOKA,    //!< Ø
    INVESTMENTS_SAPPORO,    //!< DØ
    INVESTMENTS_PTS,        //!< ÝæøVXe
};

/*!
 *  @brief  ÔÑæª
 */
enum eStockPeriodOfTime
{
    PERIOD_NONE,        //!< ³
    PERIOD_DAYTIME,     //!< fC^C
    PERIOD_NIGHTTIME,   //!< iCg^C
};

/*!
 *  @brief  ¶íÊ
 */
enum eOrderType
{
    ORDER_NONE,

    ORDER_BUY,      //!< ¢
    ORDER_SELL,     //!< è
    ORDER_CORRECT,  //!< ù³
    ORDER_CANCEL,   //!< æÁ
    ORDER_REPSELL,  //!< Ô
    ORDER_REPBUY,   //!< Ô

    NUM_ORDER,
};

/*!
 *  @brief  t·µíÊ
 */
enum eReverseOrder
{
    RV_GTE, //!< Èã(greater than or equal)
    RV_LTE, //!< Èº(less than or equal)
};

/*!
 *  @brief  ¶ð
 */
enum eOrderCondition
{
    CONDITION_NONE = 0,

    CONDITION_OPENING,      //!< ñèt«
    CONDITION_CLOSE,        //!< ø¯
    CONDITION_UNPROMOTED,   //!< s¬
};

} // namespace trading
