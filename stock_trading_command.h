/*!
 *  @file   stock_trading_command.h
 *  @brief  株取引命令
 *  @date   2017/12/27
 */
#pragma once

#include <string>

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

    /*!
     *  @param  type    命令種別
     *  @param  code    銘柄コード
     *  @param  name    銘柄名
     *  @param  tid     戦略ID
     *  @param  gid     戦略内グループID
     */
    StockTradingCommand(eType type, const StockCode& code, const std::wstring& name, int32_t tid, int32_t gid);
    /*!
     *  @brief  発注価格設定
     */
    void SetOrderParam(int32_t order_type,
                       int32_t order_condition,
                       uint32_t number,
                       float64 value,
                       bool b_leverage)
    { 
        param1 = order_type;
        param2 = order_condition;
        param3 = number;
        param4 = (b_leverage) ?1 :0;
        fparam0 = value;
    }

    /*!
     *  @brief  命令種別を得る
     */
    eType GetType() const { return m_type; }
    /*!
     *  @brief  銘柄コードを得る
     */
    uint32_t GetCode() const { return m_code; }
    /*!
     *  @brief  銘柄名(utf-16)を得る
     */
    const std::wstring& GetName() const { return m_name; }
    /*!
     *  @brief  戦略IDを得る
     */
    int32_t GetTacticsID() const { return m_tactics_id; }
    /*!
     *  @brief  グループIDを得る
     */
    int32_t GetGroupID() const { return m_group_id; }

    /*!
     *  @brief  緊急時命令か
     *  @note   緊急時命令(急騰落狙い/急変動時保険損切りetc)ならtrue
     *  @note   RESET対象外にしたい…
     */
    bool IsForEmergencyCommand() const
    {
        switch (m_type)
        {
        case ORDER:
            return param4 != 0;
        case EMERGENCY:
            return false;
        default:
            return false;
        }
    }

    /*!
     *  @brief  株注文パラメータを得る
     *  @param[out] dst 格納先
     *  @note   取引所種別は触らない(保有してないので)
     */
    void GetOrder(StockOrder& dst) const;

private:
    eType m_type;           //!< 命令種別
    uint32_t m_code;        //!< 銘柄コード
    std::wstring m_name;    //!< 銘柄名(メッセージ用/utf-16)
    int32_t m_tactics_id;   //!< 所属戦略ID
    int32_t m_group_id;     //!< 戦略内グループID

    int32_t param0;     //!< フリーパラメータ(int32) [                /              ]
    int32_t param1;     //!< フリーパラメータ(int32) [                /eOrderType    ]
    int32_t param2;     //!< フリーパラメータ(int32) [                /eOrderConditon]
    uint32_t param3;    //!< フリーパラメータ(int32) [                /株数          ]
    uint32_t param4;    //!< フリーパラメータ(int32) [緊急時命令フラグ/信用フラグ    ]

    float64 fparam0;    //!< フリーパラメータ(float) [                /発注価格      ]
};

} // namespace trading
