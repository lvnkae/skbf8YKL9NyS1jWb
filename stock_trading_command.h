/*!
 *  @file   stock_trading_command.h
 *  @brief  株取引命令
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
 *  @brief  株取引命令
 *  @note   継承用
 *  @note   取引戦略から発注管理への指示に使う
 */
class StockTradingCommand
{
public:
    enum eCommandType
    {
        NONE = 0,

        EMERGENCY,          //!< 緊急モード(通常注文全消し&通常発注一時停止)
        BUYSELL_ORDER,      //!< 売買注文(現物売買・信用新規売買)
        REPAYMENT_LEV_ORDER,//!< 信用返済注文(信用返済売買)
        CONTROL_ORDER,      //!< 制御注文(価格訂正・注文取消)
    };

    /*!
     *  @brief  命令種別を得る
     */
    eCommandType GetType() const { return m_type; }
    /*!
     *  @brief  銘柄コードを得る
     */
    uint32_t GetCode() const { return m_code; }
    /*!
     *  @brief  戦略IDを得る
     */
    int32_t GetTacticsID() const { return m_tactics_id; }

    /*!
     *  @brief  株注文命令か
     */
    virtual bool IsOrder() const { return false; }

    /*!
     *  @brief  緊急モード対象グループを得る(コピー)
     */
    virtual std::unordered_set<int32_t> GetEmergencyTargetGroup() const { return std::unordered_set<int32_t>(); }

    /*!
     *  @brief  株注文パラメータを得る(コピー)
     */
    virtual StockOrder GetOrder() const { return StockOrder(); }
    /*!
     *  @brief  戦略グループID取得
     */
    virtual int32_t GetOrderGroupID() const;
    /*!
     *  @brief  戦略注文固有ID取得
     */
    virtual int32_t GetOrderUniqueID() const;
    /*!
     *  @brief  株注文種別取得
     */
    virtual eOrderType GetOrderType() const{ return ORDER_NONE; }
    /*!
     *  @brief  株注文株数取得
     */
    virtual int32_t GetOrderNumber() const { return 0; }
    /*!
     *  @brief  株信用注文か
     */
    virtual bool IsLeverageOrder() const { return false; }
    /*!
     *  @brief  株注文番号取得
     */
    virtual int32_t GetOrderID() const;
    /*!
     *  @brief  返済注文：建日指定を得る
     */
    virtual garnet::YYMMDD GetRepLevBargainDate() const { return garnet::YYMMDD(); }
    /*!
     *  @brief  返済注文：建単価指定を得る
     */
    virtual float64 GetRepLevBargainValue() const { return 0.0; }

    /*!
     *  @brief  同属性の株注文命令か
     */
    virtual bool IsSameAttrOrder(const StockTradingCommand& right) const { return false; }
    /*!
     *  @brief  rightは同属の売買注文か？
     *  @param  right   比較する命令
     */
    virtual bool IsSameBuySellOrder(const StockTradingCommand& right) const { return false; }

    /*!
     *  @brief  株注文株数設定
     */
    virtual void SetOrderNumber(int32_t number) {}
    /*!
     *  @brief  株注文価格設定
     */
    virtual void SetOrderValue(float64 value) {}
    /*!
     *  @brief  BuySellOrder同士のコピー
     *  @param  src コピー元
     */
    virtual void CopyBuySellOrder(const StockTradingCommand& src) {}
    /*!
     *  @brief  返済注文：建日/建単価設定
     *  @param  date    建日
     *  @param  value   建単価
     */
    virtual void SetRepLevBargain(const garnet::YYMMDD& date, float64 value) {}

    virtual ~StockTradingCommand() {};

protected:
    /*!
     *  @param  type            命令種別
     *  @param  code            銘柄コード
     *  @param  tactics_id      戦略ID
     */
    StockTradingCommand(eCommandType type, const StockCode& code, int32_t tactics_id);

private:
    eCommandType m_type;    //!< 命令種別
    uint32_t m_code;        //!< 銘柄コード
    int32_t m_tactics_id;   //!< 所属戦略ID

    StockTradingCommand(const StockTradingCommand&);
    StockTradingCommand(const StockTradingCommand&&);
    StockTradingCommand& operator= (const StockTradingCommand&);
};

/*!
 *  @brief  株注文命令
 *  @note   継承用
 */
class StockTradingCommand_Order : public StockTradingCommand
{
private:
    bool IsOrder() const override { return true; }
    /*!
     *  @brief  株注文パラメータを得る(コピー)
     */
    StockOrder GetOrder() const override { return m_order; }
    /*!
     *  @brief  戦略グループID取得
     */
    int32_t GetOrderGroupID() const override { return m_group_id; }
    /*!
     *  @brief  株注文種別取得
     */
    eOrderType GetOrderType() const override { return m_order.m_type; }
    /*!
     *  @brief  株注文株数取得
     */
    int32_t GetOrderNumber() const override { return m_order.m_number; }
    /*!
     *  @brief  株信用注文か
     */
    bool IsLeverageOrder() const override { return m_order.m_b_leverage; }

    /*!
     *  @brief  株注文株数設定
     */
    void SetOrderNumber(int32_t number) override { m_order.m_number = number; }
    /*!
     *  @brief  株注文価格設定
     */
    void SetOrderValue(float64 value) override { m_order.m_value = value; }

protected:
    /*!
     *  @brief  戦略注文固有ID取得
     */
    int32_t GetOrderUniqueID() const override { return m_unique_id; }
    /*!
     *  @brief  同属性の株注文命令か
     */
    bool IsSameAttrOrder(const StockTradingCommand& right) const override;
    /*!
     *  @brief  numberを除いてStockOrderをコピー
     *  @param  src コピー元
     */
    void CopyStockOrderWithoutNumber(const StockTradingCommand& src);

    /*!
     *  @param  type        命令種別
     *  @param  code        銘柄コード
     *  @param  tactics_id  戦略ID
     *  @param  group_id    戦略グループID
     *  @param  unique_id   戦略注文固有ID
     */
    StockTradingCommand_Order(eCommandType type,
                              const StockCode& code,
                              int32_t tactics_id,
                              int32_t group_id,
                              int32_t unique_id);

    int32_t m_group_id;     //!< 戦略グループID
    int32_t m_unique_id;    //!< 戦略注文固有ID
    StockOrder m_order;     //!< 株注文パラメータ
};

/*!
 *  @brief  緊急モード命令
 */
class StockTradingCommand_Emergency : public StockTradingCommand
{
public:
    /*!
     *  @brief  コマンド生成(緊急モード)
     *  @param  code            銘柄コード
     *  @param  tactics_id      戦略ID
     *  @param  target_group    対象グループ<戦略グループID>
     */
    StockTradingCommand_Emergency(const StockCode& code,
                                  int32_t tactics_id,
                                  const std::unordered_set<int32_t>& target_group);

private:
    /*!
     *  @brief  緊急モード対象グループを得る(コピー)
     */
    std::unordered_set<int32_t> GetEmergencyTargetGroup() const override { return m_target_group; }

    //! 対象グループ
    std::unordered_set<int32_t> m_target_group;
};

/*!
 *  @brief  株売買注文命令
 *  @note   現物売買・信用新規売買
 */
class StockTradingCommand_BuySellOrder : public StockTradingCommand_Order
{
public:
    /*!
     *  @brief  コマンド作成(発注[売買])
     *  @param  investments 取引所種別
     *  @param  type        命令種別
     *  @param  code        銘柄コード
     *  @param  tactics_id  戦略ID
     *  @param  group_id    戦略グループID
     *  @param  unique_id   戦略注文固有ID
     *  @param  order_type  注文種別(eOrderType)
     *  @param  order_cond  注文条件(eOrderCondition)
     *  @param  b_leverage  信用取引フラグ
     *  @param  number      注文株数
     *  @param  value       注文価格
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
     *  @brief  rightは同属の売買注文か？
     *  @param  right   比較する命令
     */
    bool IsSameBuySellOrder(const StockTradingCommand& right) const override;
    /*!
     *  @brief  BuySellOrder同士のコピー
     *  @param  src コピー元
     */
    void CopyBuySellOrder(const StockTradingCommand& src) override;
};

/*!
 *  @brief  信用返済注文
 *  @note   信用返済売買
 */
class StockTradingCommand_RepLevOrder : public StockTradingCommand_Order
{
public:
    /*!
     *  @brief  コマンド作成(発注[信用返済])
     *  @param  investments 取引所種別
     *  @param  type        命令種別
     *  @param  code        銘柄コード
     *  @param  tactics_id  戦略ID
     *  @param  group_id    戦略グループID
     *  @param  unique_id   戦略注文固有ID
     *  @param  order_type  注文種別(eOrderType)
     *  @param  order_cond  注文条件(eOrderCondition)
     *  @param  b_leverage  信用取引フラグ
     *  @param  number      注文株数
     *  @param  value       注文価格
     *  @param  bg_date     建日
     *  @param  bg_value    建単価
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
     *  @brief  返済注文：建日指定を得る
     */
    garnet::YYMMDD GetRepLevBargainDate() const override { return m_bargain_date; }
    /*!
     *  @brief  返済注文：建単価指定を得る
     */
    float64 GetRepLevBargainValue() const override { return m_bargain_value; }
    
    /*!
     *  @brief  返済注文：建日/建単価設定
     *  @param  date    建日
     *  @param  value   建単価
     */
    void SetRepLevBargain(const garnet::YYMMDD& date, float64 value) override
    {
        m_bargain_date = date;
        m_bargain_value = value;
    }

    garnet::YYMMDD m_bargain_date;  //!< 建日
    float64 m_bargain_value;    //!< 建単価
};

/*!
 *  @brief  制御注文
 *  @note   価格訂正・注文取消
 */
class StockTradingCommand_ControllOrder : public StockTradingCommand_Order
{
public:
    /*!
     *  @brief  コマンド作成(発注[訂正/取消])
     *  @param  src_command 売買命令(この命令で上書き(訂正)、またはこの命令を取消す)
     *  @param  order_type  命令種別
     *  @param  order_id    注文番号(証券会社が発行したもの/管理用)
     */
    StockTradingCommand_ControllOrder(const StockTradingCommand& src_command,
                                      eOrderType order_type,
                                      int32_t order_id);

private:
    /*!
     *  @brief  株注文番号取得
     */
    int32_t GetOrderID() const override { return m_order_id; }

    int32_t m_order_id; //!< 注文番号(証券会社が発行したもの/管理用)
};

} // namespace trading
