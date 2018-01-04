/*!
 *  @file   stock_trading_tactics.h
 *  @brief  株取引戦略
 *  @date   2017/12/08
 */
#pragma once

#include <functional>
#include <vector>
#include <unordered_set>

struct HHMMSS;

namespace trading
{

struct StockPortfolio;
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
    enum eOrderType
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

        bool empty() const { return m_type == TRRIGER_NONE; }

        /*!
         *  @brief  判定
         *  @param  hhmmss      現在時分秒
         *  @param  valuedata   価格データ(1銘柄分)
         *  @param  script_mng  外部設定(スクリプト)管理者
         *  @retval true        トリガー発生
         */
        bool Judge(const HHMMSS& hhmmss, const StockPortfolio& valuedata, TradeAssistantSetting& script_mng) const;
    };

    /*!
     *  @brief  緊急モード設定
     */
    class Emergency
    {
    private:
        std::unordered_set<int32_t> m_group;    //!< 対象グループ番号
        Trigger m_condition;                    //!< 発生条件

    public:
        Emergency()
        : m_group()
        , m_condition()
        {
        }

        void AddTargetGroupID(int32_t group_id) { m_group.insert(group_id); }
        void SetCondition(const Trigger& trigger) { m_condition = trigger; }

        const std::unordered_set<int32_t>& RefTargetGroup() const { return m_group; }
        bool empty() const { return m_condition.empty(); }

        /*!
         *  @brief  判定
         *  @param  hhmmss      現在時分秒
         *  @param  valuedata   価格データ(1銘柄分)
         *  @param  script_mng  外部設定(スクリプト)管理者
         *  @retval true        トリガー発生
         */
        bool Judge(const HHMMSS& hhmmss, const StockPortfolio& valuedata, TradeAssistantSetting& script_mng) const
        {
            return m_condition.Judge(hhmmss, valuedata, script_mng);
        }
    };

    /*!
     *  @brief  注文設定
     */
    class Order
    {
    private:
        int32_t m_unique_id;    //!< 注文固有ID
        int32_t m_group_id;     //!< 戦略グループID(同一グループの注文は排他制御される)
        eOrderType m_type;      //!< タイプ
        int32_t m_value_func;   //!< 価格取得関数(リファレンス)
        int32_t m_volume;       //!< 株数
        bool m_b_leverage;      //!< 信用フラグ
        Trigger m_condition;    //!< 発注条件

        void SetParam(eOrderType type, bool b_leverage, int32_t func_ref, int32_t volume)
        {
            m_type = type;
            m_value_func = func_ref;
            m_volume = volume;
            m_b_leverage = b_leverage;
        }

    public:
        Order()
        : m_unique_id(0)
        , m_group_id(0)
        , m_type(eOrderType::ORDER_NONE)
        , m_volume(0)
        , m_value_func(0)
        , m_b_leverage(false)
        , m_condition()
        {
        }

        void SetUniqueID(int32_t unique_id) { m_unique_id = unique_id; }
        void SetGroupID(int32_t group_id) { m_group_id = group_id; }

        void SetBuy(bool b_leverage, int32_t func_ref, int32_t volume) { SetParam(BUY, b_leverage, func_ref, volume); }
        void SetSell(bool b_leverage, int32_t func_ref, int32_t volume) { SetParam(SELL, b_leverage, func_ref, volume); }
        void SetCondition(const Trigger& trigger) { m_condition = trigger; }

        /*!
         *  @brief  OrderTypeを得る
         */
        eOrderType GetType() const { return m_type; }
        int32_t GetGroupID() const { return m_group_id; }
        int32_t GetUniqueID() const { return m_unique_id; }
        int32_t GetVolume()  const { return m_volume; }
        bool GetIsLeverage() const { return m_b_leverage; }
        /*!
         *  @brief  価格取得関数参照取得
         */
        int32_t GetValueFuncReference() const { return m_value_func; }
        /*!
         *  @brief  判定
         *  @param  hhmmss      現在時分秒
         *  @param  valuedata   価格データ(1銘柄分)
         *  @param  script_mng  外部設定(スクリプト)管理者
         *  @retval true        トリガー発生
         */
        bool Judge(const HHMMSS& hhmmss, const StockPortfolio& valuedata, TradeAssistantSetting& script_mng) const
        {
            return m_condition.Judge(hhmmss, valuedata, script_mng);
        }
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
    void AddRepaymentOrder(const Order& order);

    /*!
     */
    typedef std::function<void(const StockTradingCommand&)> EnqueueFunc;

    /*!
     *  @brief  戦略解釈
     *  @param  hhmmss          現在時分秒
     *  @param  em_group        緊急モード対象グループ<戦略グループID>
     *  @param  valuedata       価格データ(1銘柄分)
     *  @param  script_mng      外部設定(スクリプト)管理者
     *  @param  enqueue_func    命令をキューに入れる関数
     */
    void Interpret(const HHMMSS& hhmmss,
                   const std::unordered_set<int32_t>& em_group,
                   const StockPortfolio& valuedata,
                   TradeAssistantSetting& script_mng,
                   const EnqueueFunc& enqueue_func) const;

private:
    int32_t m_unique_id;                //!< 固有ID(戦略データ間で被らない数字)
    std::vector<Emergency> m_emergency; //!< 緊急モードリスト
    std::vector<Order> m_fresh;         //!< 新規注文リスト
    std::vector<Order> m_repayment;     //!< 返済注文リスト
};

} // trading
