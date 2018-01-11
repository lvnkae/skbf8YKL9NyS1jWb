/*!
 *  @file   trade_define.h
 *  @brief  トレーディング関連定義
 *  @date   2017/05/05
 */
#pragma once

namespace trading
{

/*!
 *  @brief  取引種別
 */
enum eTradingType
{
    TYPE_NONE = 0,

    TYPE_STOCK,     //!< 株式
    TYPE_FX,        //!< 為替
    TYPE_CRYPT_CR,  //!< 暗号通貨(仮想通貨)
};

/*!
 *  @brief  証券会社種別
 */
enum eSecuritiesType
{
    SEC_NONE = 0,

    SEC_SBI,        //!< SBI証券
    SEC_KABUDOT,    //!< kabu.com
    SEC_MONEX,      //!< マネックス証券
    SEC_NOMURA,     //!< 野村證券
};

/*!
 *  @brief  株取引所種別
 */
enum eStockInvestmentsType
{
    INVESTMENTS_NONE = 0,   //!< なし

    INVESTMENTS_TOKYO,      //!< 東証
    INVESTMENTS_NAGOYA,     //!< 名証
    INVESTMENTS_FUKUOKA,    //!< 福証
    INVESTMENTS_SAPPORO,    //!< 札証
    INVESTMENTS_PTS,        //!< 私設取引システム
};

/*!
 *  @brief  注文種別
 */
enum eOrderType
{
    ORDER_NONE,

    ORDER_BUY,      //!< 買い
    ORDER_SELL,     //!< 売り
    ORDER_CORRECT,  //!< 訂正
    ORDER_CANCEL,   //!< 取消
    ORDER_REPSELL,  //!< 返売
    ORDER_REPBUY,   //!< 返買

    NUM_ORDER,
};

/*!
 *  @brief  逆差し種別
 */
enum eReverseOrder
{
    RV_GTE, //!< 以上(greater than or equal)
    RV_LTE, //!< 以下(less than or equal)
};

/*!
 *  @brief  注文条件
 */
enum eOrderCondition
{
    CONDITION_NONE = 0,

    CONDITION_OPENING,      //!< 寄り付き
    CONDITION_CLOSE,        //!< 引け
    CONDITION_UNPROMOTED,   //!< 不成
};

} // namespace trading
