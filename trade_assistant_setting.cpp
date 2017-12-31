/*!
 *  @file   trade_assistant_setting.cpp
 *  @brief  トレーディング補助：外部設定(スクリプト)
 *  @date   2017/12/09
 */
#include "trade_assistant_setting.h"

#include "environment.h"
#include "stock_trading_tactics.h"
#include "trade_struct.h"

#include "lua_accessor.h"
#include "update_message.h"
#include "utility_datetime.h"
#include "yymmdd.h"

#include "boost/algorithm/string.hpp"
#include <functional>


namespace trading
{

class TradeAssistantSetting::PIMPL
{
private:
    LuaAccessor m_lua_accessor;     //!< luaアクセサ

    eTradingType m_trading_type;    //!< 取引種別
    eSecuritiesType m_securities;   //!< 証券会社種別

    //! 無アクセスで取引サイトとのセッションを維持できる時間[分]
    int32_t m_session_keep_minute;  
    //! 株価更新(取得)間隔[秒]
    int32_t m_stock_value_inverval_second;
    //! 緊急モード維持秒数(=冷却期間)[秒]
    int32_t m_emergency_cool_second;
    //! 監視銘柄最大登録数
    int32_t m_max_portfolio_entry;
    //! 監視に使うポートフォリオ番号
    int32_t m_use_portfolio_number;

    /*!
     *  @brief  取引種別文字列から列挙子に変換
     *  @param  str 取引種別文字列
     *  @return 取引種別列挙子
     */
    static eTradingType ToTradingTypeFromString(const std::string& str)
    {
        if (str.compare("STOCK") == 0) {
            return TYPE_STOCK;
        } else if (str.compare("FX") == 0) {
            return TYPE_FX;
        } 
        return TYPE_NONE;
    }
    /*!
     *  @brief  証券会社種別文字列から列挙子に変換
     *  @param  str 証券会社種別文字列
     *  @param  証券会社種別列挙子
     */
    static eSecuritiesType ToSecuritiesTypeFromString(const std::string& str)
    {
        if (str.compare("SBI") == 0) {
            return SEC_SBI;
        } else
        if (str.compare("kabu.com") == 0) {
            return SEC_KABUDOT;
        } else
        if (str.compare("MONEX") == 0) {
            return SEC_MONEX;
        }
        return SEC_NONE;
    }

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  株取引戦略データ構築：トリガー設定1つ分
     *  @param[out] o_message
     *  @param[out] add_func    トリガー追加関数
     */
    void BuildStockTactics_TriggerUnit(UpdateMessage& o_message, const std::function<void(StockTradingTactics::Trigger&)>& add_func)
    {
        LuaAccessor& accessor = m_lua_accessor;

        std::string trigger_type_str;
        if (!accessor.GetTableParam("Type", trigger_type_str)) {
            o_message.AddErrorMessage("no trigger type.");
            return;
        }
        StockTradingTactics::Trigger trigger;
        if (trigger_type_str == "ValueGap") {
            float32 percent = 0.f;
            int32_t second = 0;
            if (!accessor.GetTableParam("Percent", percent)) {
                o_message.AddErrorMessage("no ValueGap-percent.");
                return;
            }
            if (!accessor.GetTableParam("Second", second)) {
                o_message.AddErrorMessage("no ValueGap-second.");
                return;
            }
            trigger.Set_ValueGap(percent, second);
        } else
        if (trigger_type_str == "NoContract") {
            int32_t second = 0;
            if (!accessor.GetTableParam("Second", second)) {
                o_message.AddErrorMessage("no NoContract-second.");
                return;
            }
            trigger.Set_NoContract(second);
        } else
        if (trigger_type_str == "Formula")  {
            int32_t func_ref = 0;
            if (!accessor.GetLuaFunctionReference("Formula", func_ref)) {
                o_message.AddErrorMessage("no Formula-formula.");
                return;
            }
            trigger.Set_ScriptFunction(func_ref);
        } else {
            o_message.AddErrorMessage("illegal trigger type(" + trigger_type_str + ")");
            return;
        }

        add_func(trigger);
    }
    /*!
    *  @brief  株取引戦略データ構築：発注条件
    *  @param[out] o_message
    *  @param[out] o_order      発注条件格納先
    */
    void BuildStockTactics_OrderCondition(UpdateMessage& o_message, StockTradingTactics::Order& o_order)
    {
        LuaAccessor& accessor = m_lua_accessor;

        bool is_cond = (accessor.OpenChildTable("Condition") >= 0);
        if (is_cond) {
            BuildStockTactics_TriggerUnit(o_message,
                                          [&o_order](const StockTradingTactics::Trigger& trigger)
                                            { o_order.SetCondition(trigger); }
                                         );
        }
        accessor.CloseTable();
    }   
    /*!
     *  @brief  株取引戦略データ構築：注文データ(Core)
     *  @param[out] o_message
     *  @param[out] o_type_str  注文種別文字列格納先
     *  @param[out] o_val_func  価格決定lua関数リファレンス(格納先)
     *  @param[out] o_volume    株数格納先
     *  @param[out] o_order     注文データ格納先
     *  @retval     true        成功
     */
    bool BuildStockTactics_OrderCore(UpdateMessage& o_message, std::string& o_type_str, int32_t& o_val_func, int32_t& o_volume, StockTradingTactics::Order& o_order)
    {
        LuaAccessor& accessor = m_lua_accessor;
        int32_t group_id = 0;
        if (accessor.GetTableParam("Group", group_id)) {
            // グループIDは設定がなくてもエラーではない
            o_order.SetGroupID(group_id);
        }
        if (!accessor.GetTableParam("Type", o_type_str)) {
            o_message.AddErrorMessage("no order type.");
            return false;
        }
        if (!accessor.GetLuaFunctionReference("Value", o_val_func)) {
            o_message.AddErrorMessage("no decide-value function.");
            return false;
        }
        if (!accessor.GetTableParam("Volume", o_volume)) {
            o_message.AddErrorMessage("no volume.");
            return false;
        }
        BuildStockTactics_OrderCondition(o_message, o_order);
        return true;
    }
    /*!
     *  @brief  株取引戦略データ構築：新規注文1つ分
     *  @param[out] o_message
     *  @param[out] o_tactics   戦略データ格納先
     */
    void BuildStockTactics_FreshUnit(UpdateMessage& o_message, StockTradingTactics& o_tactics)
    {
        std::string command_type_str;
        int32_t val_func = 0;
        int32_t volume = 0;
        StockTradingTactics::Order order;
        if (!BuildStockTactics_OrderCore(o_message, command_type_str, val_func, volume, order)) {
            return;
        }
        if (command_type_str == "Buy") {
            order.SetBuy(false, val_func, volume);
        } else
        if (command_type_str == "BuyLev") {
            order.SetBuy(true, val_func, volume);
        } else
        if (command_type_str == "SellLev") {
            order.SetSell(true, val_func, volume);
        } else {
            o_message.AddErrorMessage("illegal command type(" + command_type_str + ")");
            return;
        }
        o_tactics.AddFreshOrder(order);
    }
    /*!
     *  @brief  株取引戦略データ構築：返済注文1つ分
     *  @param[out] o_message
     *  @param[out] o_tactics   戦略データ格納先
     */
    void BuildStockTactics_RepaymentUnit(UpdateMessage& o_message, StockTradingTactics& o_tactics)
    {
        std::string repayment_type_str;
        int32_t val_func = 0;
        int32_t volume = 0;
        StockTradingTactics::Order order;
        if (!BuildStockTactics_OrderCore(o_message, repayment_type_str, val_func, volume, order)) {
            return;
        }
        if (repayment_type_str == "Sell") {
            order.SetSell(false, val_func, volume);
        } else
        if (repayment_type_str == "RepSell") {
            order.SetSell(true, val_func, volume);
        } else
        if (repayment_type_str == "RepBuy") {
            order.SetBuy(true, val_func, volume);
        } else {
            o_message.AddErrorMessage("illegal repayment type(" + repayment_type_str + ")");
            return;
        }
        o_tactics.AddRepaymentOrder(order);
    }

    /*!
     *  @brief  株取引戦略データ構築：戦略設定1つ分
     *  @param[out] o_message
     *  @param[out] o_tactics   戦略データ格納先
     */
    bool BuildStockTactics_TacticsUnit(UpdateMessage& o_message, StockTradingTactics& o_tactics)
    {
        LuaAccessor& accessor = m_lua_accessor;

        // 証券コード
        {
            int32_t num_code = accessor.OpenChildTable("Code");
            for (int32_t inx = 0; inx < num_code; inx++) {
                int32_t code = 0;
                if (accessor.GetArrayParam(inx, code)) {
                    StockCode scode(code);
                    if (scode.IsValid()) {
                        o_tactics.SetCode(scode.GetCode());
                    } else {
                        o_message.AddWarningMessage("illegal stock code( " + std::to_string(inx) + "_" + std::to_string(code) + ").");
                    }
                } else {
                    o_message.AddWarningMessage("illegal stock code( " + std::to_string(inx) + ").");
                }
            }
            accessor.CloseTable();
            if (num_code == 0) {
                o_message.AddWarningMessage("no stock code.");
                return false;
            }
        }
        // 発注済み注文リセット&発注処理一時停止トリガー
        {
            o_message.AddTab();
            int32_t num_reset = accessor.OpenChildTable("Emergency");
            for (int32_t inx = 0; inx < num_reset; inx++) {
                o_message.AddMessage("<EMERGENCY" + std::to_string(inx) + ">");
                accessor.OpenChildTable(inx);
                BuildStockTactics_TriggerUnit(o_message,
                                              [&o_tactics](const StockTradingTactics::Trigger& trigger)
                                                { o_tactics.AddEmergencyTrigger(trigger); }
                                             );
                accessor.CloseTable();
            }
            accessor.CloseTable();
            o_message.DecTab();
        }
        // 新規注文
        {
            o_message.AddTab();
            int32_t num_reset = accessor.OpenChildTable("Fresh");
            for (int32_t inx = 0; inx < num_reset; inx++) {
                o_message.AddMessage("<FRESG" + std::to_string(inx) + ">");
                accessor.OpenChildTable(inx);
                BuildStockTactics_FreshUnit(o_message, o_tactics);
                accessor.CloseTable();
            }
            accessor.CloseTable();
            o_message.DecTab();
        }
        // 返済注文
        {
            o_message.AddTab();
            int32_t num_reset = accessor.OpenChildTable("Repayment");
            for (int32_t inx = 0; inx < num_reset; inx++) {
                o_message.AddMessage("<REPAYMENT" + std::to_string(inx) + ">");
                accessor.OpenChildTable(inx);
                BuildStockTactics_RepaymentUnit(o_message, o_tactics);
                accessor.CloseTable();
            }
            accessor.CloseTable();
            o_message.DecTab();
        }
        return true;
    }

public:
    PIMPL()
    : m_lua_accessor()
    , m_trading_type(eTradingType::TYPE_NONE)
    , m_securities(eSecuritiesType::SEC_NONE)
    , m_session_keep_minute(0)
    , m_stock_value_inverval_second(0)
    , m_emergency_cool_second(0)
    , m_max_portfolio_entry(0)
    , m_use_portfolio_number(0)
    {
    }

    /*!
     *  @brief  トレード種別取得
     */
    eTradingType GetTradingType() const { return m_trading_type; }
    /*!
     *  @brief  証券会社種別取得
     */
    eSecuritiesType GetSecuritiesType() const { return m_securities; }
    /*!
     *  @brief  無アクセスで取引サイトとのセッションを維持できる時間
     */
    int32_t GetSessionKeepMinute() const { return m_session_keep_minute; }
    /*!
     *  @brief  株価格データ更新間隔取得
     */
    int32_t GetStockValueIntervalSecond() const { return m_stock_value_inverval_second; }
    /*!
     *  @brief  緊急モード秒数(=冷却期間)取得
     */
    int32_t GetEmergencyCoolSecond() const { return m_emergency_cool_second; }
    /*!
     *  @brief  監視銘柄最大登録数取得
     */
    int32_t GetMaxPortfolioEntry() const { return m_max_portfolio_entry; }
    /*!
     *  @brief  監視銘柄を登録するポートフォリオ番号取得
     */
    int32_t GetUsePortfolioNumber() const { return m_use_portfolio_number; }

    /*!
     *  @brief  設定読み込み
     *  @param[out] o_message
     *  @retuval    true    成功
     */
    bool ReadSetting(UpdateMessage& o_message)
    {
        o_message.AddMessage("[ReadSetting]");

        auto env = Environment::GetInstance().lock();
        if (nullptr == env) {
            o_message.AddErrorMessage("'Environment' is not created");
            return false;
        }
        const std::string setting_file(env->GetTradingScript());
        LuaAccessor& accessor = m_lua_accessor;

        if (!accessor.DoFile(setting_file)) {
            o_message.AddErrorMessage("file not found (" + setting_file + ") or syntax error.");
            return false;
        }
        {
            std::string tradingtype_str;
            if (!accessor.GetGlobalParam("TradeType", tradingtype_str)) {
                o_message.AddErrorMessage("no tradeing type.");
                return false;
            }
            m_trading_type = ToTradingTypeFromString(tradingtype_str);
        }
        {
            std::string tradingsec_str;
            if (!accessor.GetGlobalParam("TradeSec", tradingsec_str)) {
                o_message.AddErrorMessage("no securities type.");
                return false;
            }
            m_securities = ToSecuritiesTypeFromString(tradingsec_str);
        }
        if (!accessor.GetGlobalParam("SessionKeepMinute", m_session_keep_minute)) {
            o_message.AddErrorMessage("no session_keep_minute.");
            return false;
        }
        if (!accessor.GetGlobalParam("StockValueIntervalSecond", m_stock_value_inverval_second)) {
            o_message.AddErrorMessage("no stock_value_inverval_second.");
            return false;
        }
        if (!accessor.GetGlobalParam("EmergencyCoolSecond", m_emergency_cool_second)) {
            o_message.AddErrorMessage("no emergency_cool_second.");
            return false;
        }
        if (!accessor.GetGlobalParam("MaxPortfolioEntry", m_max_portfolio_entry)) {
            o_message.AddErrorMessage("no max_portfolio_entry.");
            return false;
        }
        if (!accessor.GetGlobalParam("UsePortfolioNumber", m_use_portfolio_number)) {
            o_message.AddErrorMessage("no use_portfolio_number.");
            return false;
        }

        accessor.ClearStack();
        return true;
    }

    /*!
     *  @brief  JPXの固有休業日データ構築
     *  @param[out] o_message
     *  @param[out] o_holidays  休業日データ格納先
     */
    bool BuildJPXHoliday(UpdateMessage& o_message, std::vector<MMDD>& o_holidays)
    {
        LuaAccessor& accessor = m_lua_accessor;
        o_message.AddMessage("[BuildJPXHoliday]");

        const int32_t num_holiday = accessor.OpenTable("JPXHoliday");
        o_holidays.reserve(num_holiday);
        for (int32_t inx = 0; inx < num_holiday; inx++) {
            std::string mmdd_str;
            if (accessor.GetArrayParam(inx, mmdd_str)) {
                std::vector<std::string> mmdd_split;
                boost::algorithm::split(mmdd_split, mmdd_str, boost::is_any_of("/"));
                const int32_t INX_MONTH = 0;
                const int32_t INX_DAY = 1;
                const size_t NUM_DATA = 2;
                if (NUM_DATA == mmdd_split.size()) {
                    o_holidays.emplace_back(std::stoi(mmdd_split[INX_MONTH]),
                                            std::stoi(mmdd_split[INX_DAY]));
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        accessor.CloseTable();
        return !o_holidays.empty();
    }

    /*!
     *  @brief  株取引タイムテーブル構築
     *  @param[out] o_message
     *  @param[out] o_tt        タイムテーブル格納先
     */
    bool BuildStockTimeTable(UpdateMessage& o_message, std::vector<StockTimeTableUnit>& o_tt)
    {
        LuaAccessor& accessor = m_lua_accessor;
        o_message.AddMessage("[BuildStockTimeTable]");

        o_message.AddTab();
        const int32_t num_tactics = accessor.OpenTable("StockTimeTable");
        o_tt.reserve(num_tactics);
        for (int32_t inx = 0; inx < num_tactics; inx++) {
            o_message.AddMessage("<TIMETABLE" + std::to_string(inx) + ">");
            StockTimeTableUnit tt;
            accessor.OpenChildTable(inx);
            const int32_t ARRAY_INX_STARTTIME = 0;
            const int32_t ARRAY_INX_MODE = 1;
            std::string time_str;
            if (accessor.GetArrayParam(ARRAY_INX_STARTTIME, time_str)) {
                std::string mode_str;
                if (accessor.GetArrayParam(ARRAY_INX_MODE, mode_str)) {
                    std::tm time_work;
                    if (utility::ToTimeFromString(time_str, "%H:%M", time_work)) {
                        if (tt.SetMode(mode_str)) {
                            tt.m_hhmmss.m_hour = time_work.tm_hour;
                            tt.m_hhmmss.m_minute = time_work.tm_min;
                            o_tt.push_back(tt);
                        }
                    }
                }
            }
            accessor.CloseTable();
        }
        accessor.CloseTable();
        o_message.DecTab();
        return true;
    }

    /*!
     *  @brief  株取引戦略データ構築
     *  @param[out] o_message
     *  @param[out] o_tactics   戦略データ格納先
     */
    bool BuildStockTactics(UpdateMessage& o_message, std::vector<StockTradingTactics>& o_tactics)
    {
        LuaAccessor& accessor = m_lua_accessor;
        o_message.AddMessage("[BuildStockTactics]");

        o_message.AddTab();
        const int32_t num_tactics = accessor.OpenTable("StockTactics");
        o_tactics.reserve(num_tactics);
        for (int32_t inx = 0; inx < num_tactics; inx++) {
            o_message.AddMessage("<TACTICS" + std::to_string(inx) + ">");
            StockTradingTactics tactics;
            accessor.OpenChildTable(inx);
            if (BuildStockTactics_TacticsUnit(o_message, tactics)) {
                tactics.SetUniqueID(inx);
                o_tactics.emplace_back(tactics);
            }
            accessor.CloseTable();
        }
        accessor.CloseTable();
        o_message.DecTab();
        return true;
    }

    /*!
     *  @brief  スクリプト関数呼び出し
     *  @param  func_ref    関数参照値
     *  @param              以降スクリプトに渡す引数
     */
    bool CallScriptBoolFunction(int32_t func_ref, float64 f0, float64 f1, float64 f2, float64 f3)
    {
        return m_lua_accessor.CallLuaBoolFunction(func_ref, f0, f1, f2, f3); 
    }
    float64 CallScriptFloatFunction(int32_t func_ref, float64 f0, float64 f1, float64 f2, float64 f3)
    {
        return m_lua_accessor.CallLuaFloatFunction(func_ref, f0, f1, f2, f3);
    }
};

/*!
 *  @brief
 */
TradeAssistantSetting::TradeAssistantSetting()
: m_pImpl(new PIMPL())
{
}

/*!
 *  @brief
 */
TradeAssistantSetting::~TradeAssistantSetting()
{
}

/*!
 *  @brief  設定ファイル読み込み指示
 *  @param[out] o_message
 */
bool TradeAssistantSetting::ReadSetting(UpdateMessage& o_message)
{
    return m_pImpl->ReadSetting(o_message);
}

/*!
 *  @brief  トレード種別取得
 */
eTradingType TradeAssistantSetting::GetTradingType() const
{
    return m_pImpl->GetTradingType();
}
/*!
 *  @brief  証券会社種別取得
 */
eSecuritiesType TradeAssistantSetting::GetSecuritiesType() const
{
    return m_pImpl->GetSecuritiesType();
}
/*!
 *  @brief  無アクセスで取引サイトとのセッションを維持できる時間
 */
int32_t TradeAssistantSetting::GetSessionKeepMinute() const
{
    return m_pImpl->GetSessionKeepMinute();
}
/*!
 *  @brief  株価格データ更新間隔取得
 */
int32_t TradeAssistantSetting::GetStockValueIntervalSecond() const
{
    return m_pImpl->GetStockValueIntervalSecond();
}
/*!
 *  @brief  緊急モード秒数(=冷却期間)取得
 */
int32_t TradeAssistantSetting::GetEmergencyCoolSecond() const
{
    return m_pImpl->GetEmergencyCoolSecond();
}
/*!
 *  @brief  監視銘柄最大登録数取得
 */
int32_t TradeAssistantSetting::GetMaxPortfolioEntry() const
{
    return m_pImpl->GetMaxPortfolioEntry();
}
/*!
 *  @brief  監視銘柄を登録するポートフォリオ番号取得
 */
int32_t TradeAssistantSetting::GetUsePortfolioNumber() const
{
    return m_pImpl->GetUsePortfolioNumber();
}

/*!
 *  @brief  JPXの固有休業日か
 *  @param  src 調べる日
 */
bool TradeAssistantSetting::BuildJPXHoliday(UpdateMessage& o_message, std::vector<MMDD>& o_holidays)
{
    return m_pImpl->BuildJPXHoliday(o_message, o_holidays);
}
/*!
 *  @brief  株取引タイムテーブル構築
 *  @param[out] o_message
 *  @param[out] o_holidays  休業日データ格納先
 */
bool TradeAssistantSetting::BuildStockTimeTable(UpdateMessage& o_message, std::vector<StockTimeTableUnit>& o_holidays)
{
    return m_pImpl->BuildStockTimeTable(o_message, o_holidays);
}
/*!
 *  @brief  株取引戦略データ構築
 *  @param[out] o_message
 *  @param[out] o_tactics   戦略データ格納先
 */
bool TradeAssistantSetting::BuildStockTactics(UpdateMessage& o_message, std::vector<StockTradingTactics>& o_tactics)
{
    return m_pImpl->BuildStockTactics(o_message, o_tactics);
}

/*!
 *  @brief  判定スクリプト関数呼び出し
 *  @param  func_ref    関数参照値
 *  @param              以降スクリプトに渡す引数
 *  @return 判定結果
 */
bool TradeAssistantSetting::CallJudgeFunction(int32_t func_ref, float64 f0, float64 f1, float64 f2, float64 f3)
{
    return m_pImpl->CallScriptBoolFunction(func_ref, f0, f1, f2, f3);
}
/*!
 *  @brief  値取得スクリプト関数呼び出し
 *  @param  func_ref    関数参照値
 *  @param              以降スクリプトに渡す引数
 *  @return なんか値
 */
float64 TradeAssistantSetting::CallGetValueFunction(int32_t func_ref, float64 f0, float64 f1, float64 f2, float64 f3)
{
    return m_pImpl->CallScriptFloatFunction(func_ref, f0, f1, f2, f3);
}
    
} // namespace trading
