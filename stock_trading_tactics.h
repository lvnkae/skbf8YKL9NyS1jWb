/*!
 *  @file   stock_trading_tactics.h
 *  @brief  株取引戦略
 *  @date   2017/12/08
 */
#pragma once

#include <functional>
#include <vector>

struct HHMMSS;

namespace trading
{

class StockCode;
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

        int32_t Dbg_GetType() const { return static_cast<int32_t>(m_type); }
        int32_t Dbg_GetSignedParam() const { return m_signed_param; }
        float32 Dbg_GetFloatParam() const { return m_float_param; }

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
     *  @brief  注文設定
     */
    class Order
    {
    private:
        int32_t m_group_id;     //!< 注文グループ番号(同一グループの注文は排他制御される)
        eOrderType m_type;      //!< タイプ
        int32_t m_value_func;   //!< 価格取得関数(リファレンス)
        int32_t m_volume;       //!< 株数
        bool m_b_leverage;      //!< 信用フラグ
        bool m_b_for_emergency; //!< 緊急モードでも執行する命令か
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
        : m_group_id(0)
        , m_type(eOrderType::ORDER_NONE)
        , m_volume(0)
        , m_value_func(0)
        , m_b_leverage(false)
        , m_b_for_emergency(false)
        , m_condition()
        {
        }

        void SetGroupID(int32_t group_id) { m_group_id = group_id; }

        void SetBuy(bool b_leverage, int32_t func_ref, int32_t volume) { SetParam(BUY, b_leverage, func_ref, volume); }
        void SetSell(bool b_leverage, int32_t func_ref, int32_t volume) { SetParam(SELL, b_leverage, func_ref, volume); }
        void SetCondition(const Trigger& trigger) { m_condition = trigger; }

        /*!
         *  @brief  緊急モードでも執行する命令か？
         */
        bool IsForEmergency() const { return m_b_for_emergency; }
        /*!
         *  @brief  OrderType
         */
        eOrderType GetType() const { return m_type; }
        int32_t GetVolume() const { return m_volume; }
        bool GetIsLeverage() const { return m_b_leverage; }
        int32_t GetGroupID() const { return m_group_id; }
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

        const Trigger& Dbg_RefTrigger() const { return m_condition; }
    };

    /*!
     */
    StockTradingTactics();

    /*!
     *  @brief  固有IDを得る
     */
    int32_t GetUniqueID() const { return m_unique_id; }
    /*!
     *  @brief  銘柄コード群を得る
     *  @param[out] dst 格納先
     */
    void GetCode(std::vector<StockCode>& dst) const;

    /*!
     *  @brief  固有IDをセットする
     *  @note   uniqueであるかはセットする側が保証すること
     */
    void SetUniqueID(int32_t id) { m_unique_id = id; }
    /*!
     *  @brief  銘柄コードをセットする
     *  @param  code    銘柄コード
     */
    void SetCode(uint32_t code);
    /*!
     *  @brief  緊急モードトリガーを追加する
     *  @param  trigger トリガー
     */
    void AddEmergencyTrigger(const Trigger& trigger);
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
    typedef std::function<void(int32_t, const StockTradingCommand&)> EnqueueFunc;

    /*!
     *  @brief  戦略解釈
     *  @param  b_emergency     緊急モードか
     *  @param  hhmmss          現在時分秒
     *  @param  valuedata       価格データ(1取引所分)
     *  @param  script_mng      外部設定(スクリプト)管理者
     *  @param  enqueue_func    命令をキューに入れる関数
     */
    void Interpret(bool b_emergency,
                   const HHMMSS& hhmmss,
                   const std::vector<StockPortfolio>& valuedata,
                   TradeAssistantSetting& script_mng,
                   const EnqueueFunc& enqueue_func) const;

private:
    /*!
     *  @brief  戦略解釈(1銘柄分)
     *  @param  b_emergency     緊急モードか
     *  @param  hhmmss          現在時分秒
     *  @param  valuedata       価格データ(1銘柄分)
     *  @param  script_mng      外部設定(スクリプト)管理者
     *  @param  enqueue_func    命令をキューに入れる関数
     */
    void InterpretAtCode(bool b_emergency,
                         const HHMMSS& hhmmss,
                         const StockPortfolio& valuedata,
                         TradeAssistantSetting& script_mng,
                         const EnqueueFunc& enqueue_func) const;


    int32_t m_unique_id;                //!< 固有ID(戦略データ間で被らない数字)
    std::vector<StockCode> m_code;      //!< 銘柄コード(一つの戦略を複数の銘柄で使うこともある)
    std::vector<Trigger> m_emergency;   //!< 緊急モードトリガー
    std::vector<Order> m_fresh;         //!< 新規注文リスト
    std::vector<Order> m_repayment;     //!< 返済注文リスト
};

} // trading
