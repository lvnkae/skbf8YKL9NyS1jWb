/*!
 *  @file   trade_assistor.h
 *  @brief  トレーディング補助
 *  @note   株・為替・先物などの売買を外部設定に従って実行するクラス
 *  @date   2017/05/04
 */
#pragma once

#include <string>
#include <memory>

class UpdateMessage;

namespace trading
{

class TradeAssistor
{
public:
    TradeAssistor();
    ~TradeAssistor();

    /*!
     *  @brief  外部設定読み込み
     */
    void ReadSetting();

    /*!
     *  @brief  トレード開始できるか？
     *  @retval true    開始できる
     */
    bool IsReady() const;

    /*!
     *  @brief  トレード開始
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub);

    /*!
     *  @brief  売買一時停止
     *  @note   何かあったら外部から売買を止めてしまう
     */
    void Pause();

    /*!
     *  @brief  Update関数
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update(int64_t tickCount, UpdateMessage& o_message);

private:
    TradeAssistor(const TradeAssistor&);
    TradeAssistor(TradeAssistor&&);
    TradeAssistor& operator= (const TradeAssistor&);

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
