/*!
 *  @file   stock_trading_command.h
 *  @brief  株取引命令
 *  @date   2017/12/27
 */
#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace trading
{

class StockCode;
struct StockOrder;

/*!
 *  @brief  株取引命令
 *  @note   取引戦略から発注管理への指示に使う
 */
class StockTradingCommand
{
public:
    enum eType
    {
        NONE = 0,

        EMERGENCY,  //!< 緊急モード(通常注文全消し&通常発注一時停止)
        ORDER,      //!< 発注
    };

    enum eIparam_Order
    {
        IPARAM_GROUP_ID,        //!< 戦略グループID
        IPARAM_UNIQUE_ID,       //!< 戦略注文固有ID
        IPARAM_ORDER_TYPE,      //!< 注文種別(eOrderType)
        IPARAM_ORDER_CONDITION, //!< 注文条件(eOrderConditon)
        IPARAM_B_LEVERAGE,      //!< 信用取引フラグ
        IPARAM_NUMBER,          //!< 注文株数
        IPARAM_ORDER_ID,        //!< 注文番号(証券会社が発行したもの/管理用)

        NUM_IPARAM_ORDER,       //!< signed intパラメータ数
    };

    /*!
     *  @brief  コマンド生成(緊急モード)
     *  @param  code            銘柄コード
     *  @param  tactics_id      戦略ID
     *  @param  target_group    対象グループ<戦略グループID>
     */
    StockTradingCommand(const StockCode& code,
                        int32_t tactics_id,
                        const std::unordered_set<int32_t>& target_group);
    /*!
     *  @brief  コマンド作成(発注[売買])
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
    StockTradingCommand(const StockCode& code,
                        int32_t tactics_id,
                        int32_t group_id,
                        int32_t unique_id,
                        int32_t order_type,
                        int32_t order_condition,
                        bool b_leverage,
                        int32_t number,
                        float64 value);
    /*!
     *  @brief  コマンド作成(発注[訂正])
     *  @param  src_command 上書きする売買命令
     *  @param  order_type  注文種別
     *  @param  order_id    注文番号(証券会社が発行したもの/管理用)
     */
    StockTradingCommand(const StockTradingCommand& src_command,
                        int32_t order_type,
                        int32_t order_id);
    /*!
     *  @brief  コマンド作成(発注[取消])
     *  @param  src_order   取り消す売買注文
     *  @param  tactics_id  戦略ID
     *  @param  group_id    戦略グループID
     *  @param  unique_id   戦略注文固有ID
     *  @param  order_type  注文種別(eOrderType)
     *  @param  order_id    注文番号(証券会社が発行したもの/管理用)
     */
    StockTradingCommand(const StockOrder& src_order,
                        int32_t tactics_id,
                        int32_t group_id,
                        int32_t unique_id,
                        int32_t order_type,
                        int32_t order_id);


    /*!
     *  @brief  命令種別を得る
     */
    eType GetType() const { return m_type; }
    /*!
     *  @brief  銘柄コードを得る
     */
    uint32_t GetCode() const { return m_code; }
    /*!
     *  @brief  戦略IDを得る
     */
    int32_t GetTacticsID() const { return m_tactics_id; }

    /*!
     *  @brief  緊急モード対象グループを参照する
     */
    const std::vector<int32_t>& RefEmergencyTargetGroup() const { return iparam; }
    /*!
     *  @brief  整数フリーパラメータをマージする
     *  @param  src
     */
    void MergeIntFreeparam(const std::vector<int32_t>& src);

    /*!
     *  @brief  株注文パラメータを得る
     *  @param[out] dst 格納先
     *  @note   取引所種別は触らない(保有してないので)
     */
    void GetOrder(StockOrder& dst) const;
    /*!
     *  @brief  株注文フリーパラメータ取得：戦略グループID
     */
    int32_t GetOrderGroupID() const { return (m_type == ORDER && !iparam.empty()) ?iparam[IPARAM_GROUP_ID] :-1; }
    /*!
     *  @brief  株注文フリーパラメータ取得：戦略注文固有ID
     */
    int32_t GetOrderUniqueID() const { return (m_type == ORDER && !iparam.empty()) ?iparam[IPARAM_UNIQUE_ID] :-1; }
    /*!
     *  @brief  株注文フリーパラメータ取得：注文種別(eOrderType)
     */
    int32_t GetOrderType() const { return (m_type == ORDER && !iparam.empty()) ?iparam[IPARAM_ORDER_TYPE] :0/*ORDER_NONE*/; }
    /*!
     *  @brief  株注文フリーパラメータ取得：注文番号
     */
    int32_t GetOrderID() const { return (m_type == ORDER && !iparam.empty()) ?iparam[IPARAM_ORDER_ID] :-1; }
    /*!
     *  @brief  rightは上位の売買注文か？
     *  @param  right   比較する命令
     */
    bool IsUpperBuySellOrder(const StockTradingCommand& right) const;


private:
    eType m_type;           //!< 命令種別
    uint32_t m_code;        //!< 銘柄コード
    int32_t m_tactics_id;   //!< 所属戦略ID

    //! フリーパラメータ(符号付き整数)
    std::vector<int32_t> iparam;
    //! フリーパラメータ(倍精度浮動小数)
    float64 fparam;
};

} // namespace trading
