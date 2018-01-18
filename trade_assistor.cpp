    /*!
 *  @file   trade_assistor.cpp
 *  @brief  トレーディング補助
 *  @note   株・為替・先物などの売買を外部設定に従って実行するクラス
 *  @date   2017/05/04
 */
#include "trade_assistor.h"

#include "environment.h"
#include "stock_trading_machine.h"
#include "trade_assistant_setting.h"

#include "twitter/twitter_session.h"

namespace trading
{

class TradeAssistor::PIMPL
{
private:
    enum eSequence
    {
        SEQ_ERROR,          //!< エラー停止
        SEQ_NONE,           //!< 何もしてない
        SEQ_READSETTING,    //!< 設定ファイル読み込み中
        SEQ_COMPSETTING,    //!< 設定完了(取引できるよ)
    };

    eSequence m_sequence;                       //!< シーケンス
    TradeAssistantSetting m_setting;            //!< 外部設定管理
    std::unique_ptr<TradingMachine> m_pMachine; //!< トレードマシン

    //!< twitterとのセッション
    std::shared_ptr<garnet::TwitterSessionForAuthor> m_pTwitterSession;

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  定期更新処理：初期化
     *  @param[out] o_message
     */
    void Update_Initialize(UpdateMessage& o_message)
    {
        m_sequence = SEQ_ERROR;
        //
        if (!m_setting.ReadSetting(o_message)) {
            return;
        }
        //
        switch (m_setting.GetTradingType())
        {
        case trading::TYPE_STOCK:
            // 株売買Machine作成
            m_pMachine.reset(new StockTradingMachine(m_setting, m_pTwitterSession));
            m_sequence = SEQ_COMPSETTING;
            break;
        default:
            break;
        }
    }

public:
    PIMPL()
    : m_sequence(SEQ_NONE)
    , m_setting()
    , m_pMachine()
    , m_pTwitterSession(new garnet::TwitterSessionForAuthor(Environment::GetTwitterConfig()))
    {
    }

    /*!
     *  @brief  設定ファイル読み込み指示
     */
    void ReadSetting()
    {
        m_sequence = SEQ_READSETTING;
    }

    /*!
     *  @brief  トレード開始できるか？
     *  @retval true    開始できる
     */
    bool IsReady() const
    {
        if (m_sequence == SEQ_COMPSETTING) {
            if (m_pMachine) {
                return m_pMachine->IsReady();
            }
        }
        return false;
    }

    /*!
     *  @brief  トレード開始
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
    {
        if (m_pMachine) {
            m_pMachine->Start(tickCount, uid, pwd, pwd_sub);
        }
    }

    /*!
     *  @brief  Update関数
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update(int64_t tickCount, UpdateMessage& o_message)
    {
        switch(m_sequence)
        {
        case SEQ_READSETTING:
            Update_Initialize(o_message);
            break;
        default:
            break;
        }
        if (m_pMachine) {
            m_pMachine->Update(tickCount, m_setting, o_message);
        }
    }
};

TradeAssistor::TradeAssistor()
: m_pImpl(new PIMPL())
{
}

TradeAssistor::~TradeAssistor()
{
}

/*!
 *  @brief  外部設定読み込み
 */
void TradeAssistor::ReadSetting()
{
    m_pImpl->ReadSetting();
}

/*!
 *  @brief  トレード開始できるか？
 */
bool TradeAssistor::IsReady() const
{
    return m_pImpl->IsReady();
}

/*!
 *  @brief  トレード開始
 *  @param  tickCount   経過時間[ミリ秒]
 *  @param  uid
 *  @param  pwd
 *  @param  pwd_sub
 */
void TradeAssistor::Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
{
    m_pImpl->Start(tickCount, uid, pwd, pwd_sub);
}

/*!
 *  @brief  Update関数
 *  @param[in]  tickCount   経過時間[ミリ秒]
 *  @param[out] o_message   メッセージ(格納先)
 */
void TradeAssistor::Update(int64_t tickCount, UpdateMessage& o_message)
{
    m_pImpl->Update(tickCount, o_message);
}

} // namespace trading
