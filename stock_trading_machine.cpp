/*!
 *  @file   stock_trading_machine.h
 *  @brief  売買処理：株
 *  @date   2017/05/05
 */
#include "stock_trading_machine.h"

#include "securities_session_sbi.h"
#include "stock_ordering_manager.h"
#include "stock_trading_starter_sbi.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"

#include "cipher_aes.h"
#include "holiday_investigator.h"
#include "random_generator.h"
#include "update_message.h"
#include "twitter_session.h"
#include "utility_datetime.h"
#include "yymmdd.h"

#include <codecvt>
#include <mutex>

namespace trading
{

class StockTradingMachine::PIMPL
{
private:
    enum eSequence
    {
        SEQ_ERROR,              //!< エラー停止
        SEQ_INITIALIZE,         //!< 初期化
        SEQ_READY,              //!< 準備OK
        SEQ_CLOSED_CHECK,       //!< 東証休場調査
        SEQ_TRADING,            //!< トレード主処理
        SEQ_WAIT,               //!< 任意のウェイト処理
    };

    std::mutex m_mtx;       //!< 排他制御子
    eSequence m_sequence;   //!< シーケンス

    eSecuritiesType m_securities;                                   //!< 証券会社種別
    std::shared_ptr<SecuritiesSession> m_pSecSession;               //!< 証券会社とのセッション
    garnet::TwitterSessionForAuthorPtr m_pTwSession;                //!< twitterとのセッション(メッセージ通知用)
    std::unique_ptr<StockTradingStarter> m_pStarter;                //!< 株取引スターター
    std::unique_ptr<StockOrderingManager> m_pOrderingManager;       //!< 発注管理者
    std::unique_ptr<HolidayInvestigator> m_pHolidayInvestigator;    //!< 休日調査官

    //!< JPX固有休業日(土日祝でなくとも休みになる日)
    std::vector<garnet::MMDD> m_jpx_holiday;
    //!< 株取引タイムテーブル
    std::vector<StockTimeTableUnit> m_timetable;

    garnet::RandomGenerator m_rand_gen; //!< 乱数生成器
    garnet::CipherAES m_aes_uid;        //!< 暗号uid
    garnet::CipherAES m_aes_pwd;        //!< 暗号pwd
    garnet::CipherAES m_aes_pwd_sub;    //!< 暗号pwd_sub

    int64_t m_tickcount;                        //!< 前回操作時のtickCount
    std::tm m_last_sv_time;                     //!< 最後にサーバ(証券会社および休日判定)から得た時刻
    int64_t m_last_sv_time_tick;                //!< ↑を得たtickCount
    int64_t m_wait_count_ms;                    //!< ウェイトカウント[ミリ秒]
    eSequence m_after_wait_seq;                 //!< ウェイト開けの遷移先シーケンス
    StockTimeTableUnit::eMode m_prev_tt_mode;   //!< 前回Update時のTimeTableモード
    int64_t m_last_monitoring_tick;             //!< 最後に監視銘柄情報(価格データ)を要求したtickCount
    int64_t m_last_req_exec_info_tick;          //!< 最後に当日約定情報を要求したtickCount

    const int64_t m_monitoring_interval_ms;     //!< 監視銘柄情報(価格データ)更新間隔[ミリ秒]
    const int64_t m_exec_info_interval_ms;      //!< 当日約定情報更新間隔[ミリ秒]

private:
    PIMPL();
    PIMPL(const PIMPL&&);
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  定期更新処理：初期化
     *  @param[in]  script_mng  外部設定(スクリプト)管理者
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update_Initialize(TradeAssistantSetting& script_mng, UpdateMessage& o_message)
    {
        m_sequence = SEQ_ERROR;
        //
        if (!script_mng.BuildJPXHoliday(o_message, m_jpx_holiday)) {
            return;
        }
        if (!script_mng.BuildStockTimeTable(o_message, m_timetable)) {
            return;
        }

        // タイムテーブルは時刻降順
        std::sort(m_timetable.begin(), 
                  m_timetable.end(),
                  [](const StockTimeTableUnit& left, const StockTimeTableUnit& right)->bool
            {
                return right < left;
            });

        // 証券会社ごとの初期化処理
        switch (m_securities)
        {
        case SEC_SBI:
            m_pSecSession.reset(new SecuritiesSessionSbi(script_mng));
            m_pStarter.reset(new StockTradingStarterSbi(m_pSecSession, m_pTwSession, script_mng));
            m_pOrderingManager.reset(new StockOrderingManager(m_pSecSession, m_pTwSession, script_mng));
            m_sequence = SEQ_READY;
            break;
        default:
            break;
        }
    }

    /*!
     *  @brief  定期更新処理：東証休場調査
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update_ClosedCheck(UpdateMessage& o_message)
    {
        m_sequence = SEQ_WAIT;

        m_pHolidayInvestigator.reset(new HolidayInvestigator());
        m_pHolidayInvestigator->Investigate([this](bool b_result, bool is_holiday, const std::wstring& datetime) {
            using namespace garnet;
            // http関連スレッドから呼ばれるのでlock
            std::lock_guard<std::mutex> lock(m_mtx);
            //
            const uint32_t ACCEPTABLE_DIFF_SECONDS = 10*60; // 許容される時間ズレ(10分)
            /*std::string dummy("Fri, 03 Jan 2014 21:39:11 GMT");*/
            auto pt(std::move(utility_datetime::ToLocalTimeFromRFC1123(datetime)));
            utility_datetime::ToTimeFromBoostPosixTime(pt, m_last_sv_time);
            m_last_sv_time_tick = utility_datetime::GetTickCountGeneral();
            if (ACCEPTABLE_DIFF_SECONDS >= utility_datetime::GetDiffSecondsFromLocalMachineTime(m_last_sv_time)) {
                
                //m_last_sv_time.tm_hour = 9;//
                //m_last_sv_time.tm_min = 19;//
                //m_last_sv_time.tm_sec = 25;//
                //m_last_sv_time.tm_wday = 1;
                //is_holiday = false;

                // 土日なら週明けに再調査(成否に関係なく)
                m_after_wait_seq = SEQ_CLOSED_CHECK;
                if (utility_datetime::SATURDAY == m_last_sv_time.tm_wday) {
                    const int32_t AFTER_DAY = 2;
                    m_wait_count_ms = utility_datetime::GetAfterDayLimitMS(pt, AFTER_DAY);
                } else if (utility_datetime::SUNDAY == m_last_sv_time.tm_wday) {
                    const int32_t AFTER_DAY = 1;
                    m_wait_count_ms = utility_datetime::GetAfterDayLimitMS(pt, AFTER_DAY);
                } else {
                    if (b_result) {
                        if (is_holiday || IsJPXHoliday(m_last_sv_time)) {
                            // 祝日または固有休業日 → 翌日再調査
                            const int32_t AFTER_DAY = 1;
                            m_wait_count_ms = utility_datetime::GetAfterDayLimitMS(pt, AFTER_DAY);
                        } else {
                            // 営業日 → トレード主処理へ
                            m_sequence = SEQ_TRADING;
                        }
                    } else {
                        // 調査失敗 → 10分後に再チャレンジ
                        const int32_t WAIT_MINUTES = 10;
                        m_wait_count_ms = utility_datetime::ToMiliSecondsFromMinute(WAIT_MINUTES);
                    }
                }
            } else {
                // 時間ズレがひどかったら余計なことはさせず、永遠に待たせる(緊急モード)
            }
        });
    }

    /*!
     *  @brief  タイムテーブル更新
     *  @param  tickCount   経過時間[ミリ秒]
     *  @param  now_tm      現在時刻(ファジー)
     */
    StockTimeTableUnit::eMode CorrectTimeTable(int64_t tickCount, const std::tm& now_tm)
    {
        StockTimeTableUnit::eMode prev_mode = m_prev_tt_mode;
        StockTimeTableUnit::eMode next_mode = StockTimeTableUnit::CLOSED;
        StockTimeTableUnit now_tt(now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
        {
            bool is_smallest = true;
            for (const auto& tt : m_timetable) {
                next_mode = now_tt.m_mode;
                now_tt.m_mode = tt.m_mode;
                if (tt < now_tt) {
                    is_smallest = false;
                    break;
                }
            }
            if (is_smallest) {
                now_tt.m_mode = StockTimeTableUnit::CLOSED;
            }
        }
        bool b_valid = true;
        if (prev_mode != now_tt.m_mode) {
            auto startFunc = [this](StockTimeTableUnit::eMode mode, int64_t tickCount)->bool
            {
                auto initFunc = [this](eStockInvestmentsType investments_type,
                                       const StockBrandContainer& rcv_brand)->bool
                {
                    // http関連スレッドから呼ばれるのでlock
                    std::lock_guard<std::mutex> lock(m_mtx);
                    //
                    return m_pOrderingManager->InitMonitoringBrand(investments_type, rcv_brand);
                };
                auto updateFunc = [this](const SpotTradingsStockContainer& spot,
                                         const StockPositionContainer& position)
                {
                    // http関連スレッドから呼ばれるのでlock
                    std::lock_guard<std::mutex> lock(m_mtx);
                    //
                    return m_pOrderingManager->UpdateHoldings(spot, position);
                };
                //
                const eStockInvestmentsType start_inv
                    = StockTimeTableUnit::ToInvestmentsTypeFromMode(mode);
                StockCodeContainer monitoring_code;
                m_pOrderingManager->GetMonitoringCode(monitoring_code);
                return m_pStarter->Start(tickCount,
                                         m_aes_uid, m_aes_pwd, monitoring_code, start_inv,
                                         initFunc, updateFunc);
            };
            //
            if (StockTimeTableUnit::IDLE == now_tt.m_mode) {
                // IDLEに変化したら、その次のモードの取引所で開始処理実行
                b_valid = startFunc(next_mode, tickCount);
            } else if (StockTimeTableUnit::TOKYO == now_tt.m_mode ||
                       StockTimeTableUnit::PTS == now_tt.m_mode) {
                // 売買モードに変化したら開始処理実行
                b_valid = startFunc(now_tt.m_mode, tickCount);
            }
        }
        if (b_valid) {
            return now_tt.m_mode;
        } else {
            // スタートに失敗したら余計なことはさせず、永遠に待たせる(緊急モード)
            // ※前回のスタートが終わってない場合のみ発生
            // ※超ラグってるときとか？
            m_sequence = SEQ_WAIT;
            return StockTimeTableUnit::CLOSED;
        }
    }
    /*!
     *  @brief  定期更新処理：トレード主処理
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[in]  script_mng  外部設定(スクリプト)管理者
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update_MainTrade(int64_t tickCount, TradeAssistantSetting& script_mng, UpdateMessage& o_message)
    {
        // サーバタイムにローカルの経過時間を加えたファジーな現在時刻
        std::tm now_tm;
        const int64_t diff_tick = tickCount-m_last_sv_time_tick;
        garnet::utility_datetime::AddTimeAndDiffMS(m_last_sv_time, diff_tick, now_tm);
        //
        auto now_mode = CorrectTimeTable(tickCount, now_tm);

        if (StockTimeTableUnit::TOKYO == now_mode || StockTimeTableUnit::PTS == now_mode) {
            const eStockInvestmentsType investments_type
                = StockTimeTableUnit::ToInvestmentsTypeFromMode(now_mode);
            if (m_pStarter->IsReady()) {
                // 監視銘柄情報更新
                if ((tickCount - m_last_monitoring_tick) > m_monitoring_interval_ms) {
                    m_last_monitoring_tick = tickCount;
                    m_pSecSession->UpdateValueData(
                        [this, investments_type](bool b_success,
                                                 const std::wstring& sendtime,
                                                 const std::vector<RcvStockValueData>& rcv_valuedata) {
                        if (b_success) {
                            // http関連スレッドから呼ばれるのでlock
                            std::lock_guard<std::mutex> lock(m_mtx); 
                            //
                            m_pOrderingManager->UpdateValueData(investments_type,
                                                                sendtime,
                                                                rcv_valuedata);
                        }
                    });
                }
                // 当日約定情報更新
                if ((tickCount - m_last_req_exec_info_tick) > m_exec_info_interval_ms) {
                    m_last_req_exec_info_tick = tickCount;
                    m_pSecSession->UpdateExecuteInfo([this](bool b_success,
                                                            const std::vector<StockExecInfoAtOrder>& rcv_info) {
                        if (b_success) {
                            // http関連スレッドから呼ばれるのでlock
                            std::lock_guard<std::mutex> lock(m_mtx); 
                            //
                            m_pOrderingManager->UpdateExecInfo(rcv_info);
                        }
                    });
                }
                // 発注管理定期更新
                const garnet::HHMMSS hhmmss(now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
                m_pOrderingManager->Update(tickCount, hhmmss, investments_type, m_aes_pwd_sub, script_mng);
            }
        }

        m_prev_tt_mode = now_mode;
    }

    /*!
     *  @brief  定期更新処理：汎用ウェイト処理
     *  @param  tickCount   経過時間[ミリ秒]
     */
    void Update_Wait(int64_t tickCount)
    {
        if (m_wait_count_ms > 0) {
            const int64_t past_tick = tickCount - m_tickcount;
            if (past_tick >= m_wait_count_ms) {
                m_wait_count_ms = 0;
                m_sequence = m_after_wait_seq;
            } else {
                m_wait_count_ms -= past_tick;
            }
        }
    }

    /*!
     *  @brief  JPXの固有休業日か
     *  @param  src 調べる日
     *  @retval true    休業日だ
     */
    bool IsJPXHoliday(const std::tm& src) const
    {
        const garnet::MMDD srcmmdd(src);
        for (const auto& mmdd: m_jpx_holiday) {
            if (mmdd == srcmmdd) {
                return true;
            }
        }
        return false;
    }

public:
    /*!
     *  @param  script_mng  外部設定(スクリプト)管理者
     *  @param  tw_session  twitterとのセッション
     */
    PIMPL(const TradeAssistantSetting& script_mng,
          const garnet::TwitterSessionForAuthorPtr& tw_session)
    : m_mtx()
    , m_sequence(SEQ_INITIALIZE)
    , m_securities(script_mng.GetSecuritiesType())
    , m_pSecSession()
    , m_pTwSession(tw_session)
    , m_pStarter()
    , m_pOrderingManager()
    , m_pHolidayInvestigator()
    , m_jpx_holiday()
    , m_timetable()
    , m_rand_gen()
    , m_aes_uid()
    , m_aes_pwd()
    , m_aes_pwd_sub()
    , m_tickcount(0)
    , m_last_sv_time()
    , m_last_sv_time_tick(0)
    , m_wait_count_ms(0)
    , m_after_wait_seq(SEQ_ERROR)
    , m_prev_tt_mode(StockTimeTableUnit::CLOSED)
    , m_last_monitoring_tick(0)
    , m_monitoring_interval_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(
            script_mng.GetStockMonitoringIntervalSecond()))
    , m_exec_info_interval_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(
            script_mng.GetStockExecInfoIntervalSecond()))
    {
        memset(reinterpret_cast<void*>(&m_last_sv_time), 0, sizeof(m_last_sv_time));
    }

    /*!
     *  @brief  トレード開始できるか？
     *  @retval true    開始できる
     */
    bool IsReady() const
    {
        return m_sequence == SEQ_READY;
    }

    /*!
     *  @brief  トレード開始
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
    {
        // 暗号化して保持
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        m_aes_uid.Encrypt(m_rand_gen, cv.to_bytes(uid));
        m_aes_pwd.Encrypt(m_rand_gen, cv.to_bytes(pwd));
        m_aes_pwd_sub.Encrypt(m_rand_gen, cv.to_bytes(pwd_sub));
        // 休場チェックへ
        m_sequence = SEQ_CLOSED_CHECK;
    }

    /*!
     *  @brief  Update関数
     *  @param[in]  tickCount   経過時間[ミリ秒]
     *  @param[in]  script_mng  外部設定(スクリプト)管理者
     *  @param[out] o_message   メッセージ(格納先)
     */
    void Update(int64_t tickCount, TradeAssistantSetting& script_mng,  UpdateMessage& o_message)
    {
        std::lock_guard<std::mutex> lock(m_mtx); // Update中は受信割り込み禁止

        switch(m_sequence)
        {
        case SEQ_INITIALIZE:
            Update_Initialize(script_mng, o_message);
            break;
        case SEQ_CLOSED_CHECK:
            Update_ClosedCheck(o_message);
            break;
        case SEQ_TRADING:
            Update_MainTrade(tickCount, script_mng, o_message);
            break;
        case SEQ_WAIT:
            Update_Wait(tickCount);
        default:
            break;
        }

        m_tickcount = tickCount;
    }
};

/*!
 *  @param  script_mng  外部設定(スクリプト)管理者
 *  @param  tw_session  twitterとのセッション
 */
StockTradingMachine::StockTradingMachine(const TradeAssistantSetting& script_mng,
                                         const garnet::TwitterSessionForAuthorPtr& tw_session)
: TradingMachine()
, m_pImpl(new PIMPL(script_mng, tw_session))
{
}
/*!
 */
StockTradingMachine::~StockTradingMachine()
{
}

/*!
 *  @brief  トレード開始できるか？
 */
bool StockTradingMachine::IsReady() const
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
void StockTradingMachine::Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
{
    m_pImpl->Start(uid, pwd, pwd_sub);
}

/*!
 *  @brief  Update関数
 *  @param[in]  tickCount   経過時間[ミリ秒]
 *  @param[in]  script_mng  外部設定(スクリプト)管理者
 *  @param[out] o_message   メッセージ(格納先)
 */
void StockTradingMachine::Update(int64_t tickCount, TradeAssistantSetting& script_mng, UpdateMessage& o_message)
{
    m_pImpl->Update(tickCount, script_mng, o_message);
}

} // namespace trading
