/*!
 *  @file   stock_trading_tactics.h
 *  @brief  株取引戦略
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
*  @brief  株取引戦略
*/
class StockTradingTactics
{
private:
    enum eTriggerType
    {
        TRRIGER_NONE,

        VALUE_GAP,      // 急変動(株価が時間t以内に割合r変化(r>0:上昇/r<0:下落)
        NO_CONTRACT,    // 無約定間隔(時間t以上約定がなかった)
        SCRIPT_FUNCTION,// スクリプト関数判定
    };
    enum eTacticsOrder
    {
        ORDER_NONE,

        BUY,    // 買い注文
        SELL,   // 売り注文
    };

public:
    /*!
     *  @brief  トリガー設定
     *  @note   何某かを起こすタイミング
     */
    class Trigger
    {
    private:
        eTriggerType m_type;       //!< タイプ
        float32 m_float_param;     //!< フリーパラメータ(32bit浮動小数点)
        int32_t m_signed_param;    //!< フリーパラメータ(32bit符号付き)
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
         *  @brief  判定
         *  @param  now_time    現在時分秒
         *  @param  sec_time    現セクション開始時刻
         *  @param  valuedata   価格データ(1銘柄分)
         *  @param  script_mng  外部設定(スクリプト)管理者
         *  @retval true        トリガー発生
         */
        bool Judge(const garnet::HHMMSS& now_time,
                   const garnet::HHMMSS& sec_time,
                   const StockValueData& valuedata,
                   TradeAssistantSetting& script_mng) const;
    };

    /*!
     *  @brief  緊急モード設定
     */
    class Emergency : public Trigger
    {
    private:
        std::unordered_set<int32_t> m_group;    //!< 対象グループ番号

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
     *  @brief  注文設定
     */
    class Order : public Trigger
    {
    private:
        int32_t m_unique_id;    //!< 注文固有ID
        int32_t m_group_id;     //!< 戦略グループID(同一グループの注文は排他制御される)
        eTacticsOrder m_type;   //!< タイプ
        int32_t m_value_func;   //!< 価格取得関数(リファレンス)
        int32_t m_number;       //!< 株数
        bool m_b_leverage;      //!< 信用フラグ

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
         *  @brief  価格取得関数参照取得
         */
        int32_t GetValueFuncReference() const { return m_value_func; }
    };

    /*!
     *  @brief  返済注文設定
     */
    class RepOrder : public Order
    {
    private:
        garnet::YYMMDD m_bargain_date;  //! 建日
        float64 m_bargain_value;        //! 建単価
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
     *  @brief  固有IDを得る
     */
    int32_t GetUniqueID() const { return m_unique_id; }

    /*!
     *  @brief  固有IDをセットする
     *  @note   uniqueであるかはセットする側が保証すること
     */
    void SetUniqueID(int32_t id) { m_unique_id = id; }
    /*!
     *  @brief  緊急モードを追加する
     *  @param  emergency   緊急モード設定
     */
    void AddEmergencyMode(const Emergency& emergency);
    /*!
     *  @brief  新規注文を追加
     *  @param  order   注文設定
     */
    void AddFreshOrder(const Order& order);
    /*!
     *  @brief  返済注文を追加
     *  @param  order   注文設定
     */
    void AddRepaymentOrder(const RepOrder& order);

    /*!
     */
    typedef std::function<void(const StockTradingCommandPtr&)> EnqueueFunc;

    /*!
     *  @brief  戦略解釈
     *  @param  investments     現在取引所種別
     *  @param  now_time        現在時分秒
     *  @param  sec_time        現セクション開始時刻
     *  @param  em_group        緊急モード対象グループ<戦略グループID>
     *  @param  valuedata       価格データ(1銘柄分)
     *  @param  script_mng      外部設定(スクリプト)管理者
     *  @param  enqueue_func    命令をキューに入れる関数
     */
    void Interpret(eStockInvestmentsType investments,
                   const garnet::HHMMSS& now_time,
                   const garnet::HHMMSS& sec_time,
                   const std::unordered_set<int32_t>& em_group,
                   const StockValueData& valuedata,
                   TradeAssistantSetting& script_mng,
                   const EnqueueFunc& enqueue_func) const;

private:
    int32_t m_unique_id;                //!< 固有ID(戦略データ間で被らない数字)
    std::vector<Emergency> m_emergency; //!< 緊急モードリスト
    std::vector<Order> m_fresh;         //!< 新規注文リスト
    std::vector<RepOrder> m_repayment;  //!< 返済注文リスト
};

} // trading
