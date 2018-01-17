/*!
 *  @file   trade_assistant_setting.h
 *  @brief  トレーディング補助：外部設定(スクリプト)
 *  @date   2017/12/09
 *  @note   スクリプトへのアクセス仲介
 *  @note   luaアクセサを隠蔽する(予定はないがlua以外も使えるように)
 */
#pragma once

#include "trade_define.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace garnet { struct MMDD; }
class UpdateMessage;

namespace trading
{
struct StockTimeTableUnit;
class StockTradingTactics;

class TradeAssistantSetting
{
public:
    TradeAssistantSetting();
    ~TradeAssistantSetting();

    /*!
     *  @brief  設定ファイル読み込み
     *  @param[out] o_message
     *  @retval true    成功
     */
    bool ReadSetting(UpdateMessage& o_message);

    /*!
     *  @brief  トレード種別取得
     */
    eTradingType GetTradingType() const;
    /*!
     *  @brief  証券会社種別取得
     */
    eSecuritiesType GetSecuritiesType() const;
    /*!
     *  @brief  無アクセスで取引サイトとのセッションを維持できる時間
     *  @return 維持時間[分]
     */
    int32_t GetSessionKeepMinute() const;
    /*!
     *  @brief  緊急モード期間取得
     *  @return 冷却期間[秒]
     */
    int32_t GetEmergencyCoolSecond() const;
    /*!
     *  @brief  株価格データ更新間隔取得
     *  @return 更新間隔[秒]
     */
    int32_t GetStockMonitoringIntervalSecond() const;
    /*!
     *  @brief  当日約定情報更新間隔取得
     *  @return 更新間隔[秒]
     */
    int32_t GetStockExecInfoIntervalSecond() const;
    /*!
     *  @brief  監視銘柄最大登録数取得
     */
    int32_t GetMaxMonitoringCodeRegister() const;
    /*!
     *  @brief  監視銘柄情報出力ディレクトリ取得
     */
    std::string GetStockMonitoringLogDir() const;
    /*!
     *  @brief  銘柄監視に使用するポートフォリオ番号取得
     */
    int32_t GetUsePortfolioNumberForMonitoring() const;
    /*!
     *  @brief  ポートフォリオ表示形式(監視銘柄用)取得
     */
    int32_t GetPortfolioIndicateForMonitoring() const;
    /*!
     *  @brief  ポートフォリオ表示形式(保有銘柄用)取得
     */
    int32_t GetPortfolioIndicateForOwned() const;

    /*!
     *  @brief  JPXの固有休業日データ構築
     *  @param[out] o_message
     *  @param[out] o_holidays  休業日データ格納先
     *  @retval true    成功
     *  @note   luaにアクセスする都合上constにできない
     */
    bool BuildJPXHoliday(UpdateMessage& o_message, std::vector<garnet::MMDD>& o_holidays);
    /*!
     *  @brief  株取引タイムテーブル構築
     *  @param[out] o_message
     *  @param[out] o_tt        タイムテーブル格納先
     *  @retval true    成功
     *  @note   luaにアクセスする都合上constにできない
     */
    bool BuildStockTimeTable(UpdateMessage& o_message, std::vector<StockTimeTableUnit>& o_tt);
    /*!
     *  @brief  株取引戦略データ構築
     *  @param[out] o_message
     *  @param[out] o_tactics   戦略データ格納先
     *  @param[out] o_link      紐付け情報格納先
     *  @retval true    成功
     *  @note   luaにアクセスする都合上constにできない
     */
    bool BuildStockTactics(UpdateMessage& o_message,
                           std::unordered_map<int32_t, StockTradingTactics>& o_tactics,
                           std::vector<std::pair<uint32_t, int32_t>>& o_link);

    /*!
     *  @brief  判定スクリプト関数呼び出し
     *  @param  func_ref    関数参照値
     *  @param              以降スクリプトに渡す引数
     *  @return 判定結果
     */
    bool CallJudgeFunction(int32_t func_ref, float64, float64, float64, float64);
    /*!
     *  @brief  値取得スクリプト関数呼び出し
     *  @param  func_ref    関数参照値
     *  @param              以降スクリプトに渡す引数
     *  @return なんか値
     */
    float64 CallGetValueFunction(int32_t func_ref, float64, float64, float64, float64);

private:
    TradeAssistantSetting(const TradeAssistantSetting&);
    TradeAssistantSetting& operator= (const TradeAssistantSetting&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
