/*!
 *  @file   trading_machine.h
 *  @brief  売買処理：ベース
 *  @date   2017/05/05
 */
#pragma once

#include <string>

class UpdateMessage;

namespace trading
{
class TradeAssistantSetting;

class TradingMachine
{
public:
    virtual ~TradingMachine();

    /*!
     *  @brief  トレード開始できるか？
     *  @retval true    開始できる
     */
    virtual bool IsReady() const { return false; }

    /*!
     *  @brief  トレード開始
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    virtual void Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub) = 0;

    /*!
     *  @brief  売買一時停止
     */
    virtual void Pause() = 0;

    /*!
     *  @brief  ログ出力
     */
    virtual void OutputLog() = 0;

    /*!
     *  @brief  Update関数
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[in]  script_mng  外部設定(スクリプト)管理者
     *  @param[out] o_message   メッセージ(格納先)
     */
    virtual void Update(int64_t tickCount, TradeAssistantSetting& script_mng, UpdateMessage& o_message) = 0;

protected:
    TradingMachine();

private:
    TradingMachine(const TradingMachine&);
    TradingMachine(TradingMachine&&);
    TradingMachine& operator= (const TradingMachine&);
};

} // namespace trading
