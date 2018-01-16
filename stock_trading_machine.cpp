/*!
 *  @file   stock_trading_machine.h
 *  @brief  ���������F��
 *  @date   2017/05/05
 */
#include "stock_trading_machine.h"

#include "securities_session_sbi.h"
#include "stock_ordering_manager.h"
#include "stock_trading_starter_sbi.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"
#include "update_message.h"

#include "cipher_aes.h"
#include "holiday_investigator.h"
#include "random_generator.h"
#include "twitter/twitter_session.h"
#include "utility/utility_datetime.h"
#include "yymmdd.h"
#include "garnet_time.h"

#include <codecvt>
#include <mutex>

namespace trading
{

class StockTradingMachine::PIMPL
{
private:
    enum eSequence
    {
        SEQ_ERROR,              //!< �G���[��~
        SEQ_INITIALIZE,         //!< ������
        SEQ_READY,              //!< ����OK
        SEQ_CLOSED_CHECK,       //!< ���؋x�꒲��
        SEQ_TRADING,            //!< �g���[�h�又��
        SEQ_WAIT,               //!< �C�ӂ̃E�F�C�g����
    };

    std::mutex m_mtx;       //!< �r������q
    eSequence m_sequence;   //!< �V�[�P���X

    eSecuritiesType m_securities;                                   //!< �،���Ў��
    SecuritiesSessionPtr m_pSecSession;               //!< �،���ЂƂ̃Z�b�V����
    garnet::TwitterSessionForAuthorPtr m_pTwSession;                //!< twitter�Ƃ̃Z�b�V����(���b�Z�[�W�ʒm�p)
    std::unique_ptr<StockTradingStarter> m_pStarter;                //!< ������X�^�[�^�[
    std::unique_ptr<StockOrderingManager> m_pOrderingManager;       //!< �����Ǘ���
    std::unique_ptr<HolidayInvestigator> m_pHolidayInvestigator;    //!< �x��������

    //!< JPX�ŗL�x�Ɠ�(�y���j�łȂ��Ƃ��x�݂ɂȂ��)
    std::vector<garnet::MMDD> m_jpx_holiday;
    //!< ������^�C���e�[�u��
    std::vector<StockTimeTableUnit> m_timetable;

    garnet::RandomGenerator m_rand_gen;     //!< ����������
    garnet::CipherAES_string m_aes_uid;     //!< �Í�uid
    garnet::CipherAES_string m_aes_pwd;     //!< �Í�pwd
    garnet::CipherAES_string m_aes_pwd_sub; //!< �Í�pwd_sub

    int64_t m_tickcount;                        //!< �O�񑀍쎞��tickCount
    garnet::sTime m_last_sv_time;               //!< �Ō�ɃT�[�o(�،���Ђ���ыx������)���瓾������
    int64_t m_last_sv_time_tick;                //!< ���𓾂�tickCount
    int64_t m_wait_count_ms;                    //!< �E�F�C�g�J�E���g[�~���b]
    eSequence m_after_wait_seq;                 //!< �E�F�C�g�J���̑J�ڐ�V�[�P���X
    StockTimeTableUnit::eMode m_prev_tt_mode;   //!< �O��Update����TimeTable���[�h
    int64_t m_last_monitoring_tick;             //!< �Ō�ɊĎ��������(���i�f�[�^)��v������tickCount
    int64_t m_last_req_exec_info_tick;          //!< �Ō�ɓ���������v������tickCount
    bool m_lock_update_margin;                  //!< �]�͍X�V���b�N�t���O

    const int64_t m_monitoring_interval_ms;     //!< �Ď��������(���i�f�[�^)�X�V�Ԋu[�~���b]
    const int64_t m_exec_info_interval_ms;      //!< ���������X�V�Ԋu[�~���b]
    const int64_t m_margin_interval_ms;         //!< �]�͍X�V�Ԋu[�~���b]

private:
    PIMPL();
    PIMPL(const PIMPL&&);
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  ����X�V�����F������
     *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
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

        // �^�C���e�[�u���͎����~��
        std::sort(m_timetable.begin(), 
                  m_timetable.end(),
                  [](const StockTimeTableUnit& left, const StockTimeTableUnit& right)->bool
            {
                return right < left;
            });

        // �،���Ђ��Ƃ̏���������
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
     *  @brief  ����X�V�����F���؋x�꒲��
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
     */
    void Update_ClosedCheck(UpdateMessage& o_message)
    {
        m_sequence = SEQ_WAIT;

        m_pHolidayInvestigator.reset(new HolidayInvestigator());
        m_pHolidayInvestigator->Investigate([this](bool b_result, bool is_holiday, const std::wstring& datetime) {
            using namespace garnet;
            // http�֘A�X���b�h����Ă΂��̂�lock
            std::lock_guard<std::mutex> lock(m_mtx);
            //
            const uint32_t ACCEPTABLE_DIFF_SECONDS = 10*60; // ���e����鎞�ԃY��(10��)
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

                // �y���Ȃ�T�����ɍĒ���(���ۂɊ֌W�Ȃ�)
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
                            // �j���܂��͌ŗL�x�Ɠ� �� �����Ē���
                            const int32_t AFTER_DAY = 1;
                            m_wait_count_ms = utility_datetime::GetAfterDayLimitMS(pt, AFTER_DAY);
                        } else {
                            // �c�Ɠ� �� �g���[�h�又����
                            m_sequence = SEQ_TRADING;
                        }
                    } else {
                        // �������s �� 10����ɍă`�������W
                        const int32_t WAIT_MINUTES = 10;
                        m_wait_count_ms = utility_datetime::ToMiliSecondsFromMinute(WAIT_MINUTES);
                    }
                }
            } else {
                // ���ԃY�����Ђǂ�������]�v�Ȃ��Ƃ͂������A�i���ɑ҂�����(�ً}���[�h)
            }
        });
    }

    /*!
     *  @brief  �^�C���e�[�u���X�V
     *  @param  tickCount   �o�ߎ���[�~���b]
     *  @param  now_tm      ���ݎ���(�t�@�W�[)
     */
    StockTimeTableUnit::eMode CorrectTimeTable(int64_t tickCount, const garnet::sTime& now_tm)
    {
        StockTimeTableUnit::eMode prev_mode = m_prev_tt_mode;
        StockTimeTableUnit::eMode next_mode = StockTimeTableUnit::CLOSED;
        StockTimeTableUnit now_tt(now_tm);
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
                    // http�֘A�X���b�h����Ă΂��̂�lock
                    std::lock_guard<std::mutex> lock(m_mtx);
                    //
                    return m_pOrderingManager->InitMonitoringBrand(investments_type, rcv_brand);
                };
                auto updateFunc = [this](const SpotTradingsStockContainer& spot,
                                         const StockPositionContainer& position)
                {
                    // http�֘A�X���b�h����Ă΂��̂�lock
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
                // IDLE�ɕω�������A���̎��̃��[�h�̎�����ŊJ�n�������s
                b_valid = startFunc(next_mode, tickCount);
            } else if (StockTimeTableUnit::TOKYO == now_tt.m_mode ||
                       StockTimeTableUnit::PTS == now_tt.m_mode) {
                // �������[�h�ɕω�������J�n�������s
                b_valid = startFunc(now_tt.m_mode, tickCount);
            }
        }
        if (b_valid) {
            return now_tt.m_mode;
        } else {
            // �X�^�[�g�Ɏ��s������]�v�Ȃ��Ƃ͂������A�i���ɑ҂�����(�ً}���[�h)
            // ���O��̃X�^�[�g���I����ĂȂ��ꍇ�̂ݔ���
            // �������O���Ă�Ƃ��Ƃ��H
            m_sequence = SEQ_WAIT;
            return StockTimeTableUnit::CLOSED;
        }
    }
    /*!
     *  @brief  ����X�V�����F�g���[�h�又��
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
     */
    void Update_MainTrade(int64_t tickCount, TradeAssistantSetting& script_mng, UpdateMessage& o_message)
    {
        // �T�[�o�^�C���Ƀ��[�J���̌o�ߎ��Ԃ��������t�@�W�[�Ȍ��ݎ���
        garnet::sTime now_tm;
        const int64_t diff_tick = tickCount-m_last_sv_time_tick;
        garnet::utility_datetime::AddTimeAndDiffMS(m_last_sv_time, diff_tick, now_tm);
        //
        auto now_mode = CorrectTimeTable(tickCount, now_tm);

        if (StockTimeTableUnit::TOKYO == now_mode || StockTimeTableUnit::PTS == now_mode) {
            const eStockInvestmentsType investments_type
                = StockTimeTableUnit::ToInvestmentsTypeFromMode(now_mode);
            if (m_pStarter->IsReady()) {
                // �Ď��������X�V
                if ((tickCount - m_last_monitoring_tick) > m_monitoring_interval_ms) {
                    m_last_monitoring_tick = tickCount;
                    m_pSecSession->UpdateValueData(
                        [this, investments_type](bool b_success,
                                                 const std::wstring& sendtime,
                                                 const std::vector<RcvStockValueData>& rcv_valuedata) {
                        if (b_success) {
                            // http�֘A�X���b�h����Ă΂��̂�lock
                            std::lock_guard<std::mutex> lock(m_mtx); 
                            //
                            m_pOrderingManager->UpdateValueData(investments_type,
                                                                sendtime,
                                                                rcv_valuedata);
                        }
                    });
                }
                // ���������X�V
                if ((tickCount - m_last_req_exec_info_tick) > m_exec_info_interval_ms) {
                    m_last_req_exec_info_tick = tickCount;
                    m_pSecSession->UpdateExecuteInfo([this](bool b_success,
                                                            const std::vector<StockExecInfoAtOrder>& rcv_info) {
                        if (b_success) {
                            // http�֘A�X���b�h����Ă΂��̂�lock
                            std::lock_guard<std::mutex> lock(m_mtx); 
                            //
                            m_pOrderingManager->UpdateExecInfo(rcv_info);
                        }
                    });
                }
                // �]�͍X�V(�Z�b�V�����ێ��ړI)
                if (!m_lock_update_margin) {
                    if ((tickCount - m_pSecSession->GetLastAccessTime()) > m_margin_interval_ms) {
                        m_lock_update_margin = true;
                        m_pSecSession->UpdateMargin([this](bool b_result) {
                            // http�֘A�X���b�h����Ă΂��̂�lock
                            std::lock_guard<std::mutex> lock(m_mtx);
                            //
                            if (b_result) {
                                m_lock_update_margin = false;
                            } else {
                                m_pTwSession->Tweet(std::wstring(), L"�،��T�C�g�Ƃ̐ڑ����؂�܂���");
                            }
                        });
                    }
                }
                // �����Ǘ�����X�V
                m_pOrderingManager->Update(tickCount, now_tm, investments_type, m_aes_pwd_sub, script_mng);
            }
        }

        m_prev_tt_mode = now_mode;
    }

    /*!
     *  @brief  ����X�V�����F�ėp�E�F�C�g����
     *  @param  tickCount   �o�ߎ���[�~���b]
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
     *  @brief  JPX�̌ŗL�x�Ɠ���
     *  @param  src ���ׂ��
     *  @retval true    �x�Ɠ���
     */
    bool IsJPXHoliday(const garnet::sTime& src) const
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
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param  tw_session  twitter�Ƃ̃Z�b�V����
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
    , m_last_req_exec_info_tick(0)
    , m_lock_update_margin(false)
    , m_monitoring_interval_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(
            script_mng.GetStockMonitoringIntervalSecond()))
    , m_exec_info_interval_ms(
        garnet::utility_datetime::ToMiliSecondsFromSecond(
            script_mng.GetStockExecInfoIntervalSecond()))
    , m_margin_interval_ms(
        garnet::utility_datetime::ToMiliSecondsFromMinute(
            script_mng.GetSessionKeepMinute())/2) // �Z�b�V�����ێ��ړI�Ȃ̂ŃZ�b�V�����^�C���̔������炢��
    {
        memset(reinterpret_cast<void*>(&m_last_sv_time), 0, sizeof(m_last_sv_time));
    }

    /*!
     *  @brief  �g���[�h�J�n�ł��邩�H
     *  @retval true    �J�n�ł���
     */
    bool IsReady() const
    {
        return m_sequence == SEQ_READY;
    }

    /*!
     *  @brief  �g���[�h�J�n
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
    {
        // �Í������ĕێ�
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
        m_aes_uid.Encrypt(m_rand_gen, cv.to_bytes(uid));
        m_aes_pwd.Encrypt(m_rand_gen, cv.to_bytes(pwd));
        m_aes_pwd_sub.Encrypt(m_rand_gen, cv.to_bytes(pwd_sub));
        // �x��`�F�b�N��
        m_sequence = SEQ_CLOSED_CHECK;
    }

    /*!
     *  @brief  Update�֐�
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
     */
    void Update(int64_t tickCount, TradeAssistantSetting& script_mng,  UpdateMessage& o_message)
    {
        std::lock_guard<std::mutex> lock(m_mtx); // Update���͎�M���荞�݋֎~

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
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 *  @param  tw_session  twitter�Ƃ̃Z�b�V����
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
 *  @brief  �g���[�h�J�n�ł��邩�H
 */
bool StockTradingMachine::IsReady() const
{
    return m_pImpl->IsReady();
}

/*!
 *  @brief  �g���[�h�J�n
 *  @param  tickCount   �o�ߎ���[�~���b]
 *  @param  uid
 *  @param  pwd
 *  @param  pwd_sub
 */
void StockTradingMachine::Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
{
    m_pImpl->Start(uid, pwd, pwd_sub);
}

/*!
 *  @brief  Update�֐�
 *  @param[in]  tickCount   �o�ߎ���[�~���b]
 *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 *  @param[out] o_message   ���b�Z�[�W(�i�[��)
 */
void StockTradingMachine::Update(int64_t tickCount, TradeAssistantSetting& script_mng, UpdateMessage& o_message)
{
    m_pImpl->Update(tickCount, script_mng, o_message);
}

} // namespace trading
