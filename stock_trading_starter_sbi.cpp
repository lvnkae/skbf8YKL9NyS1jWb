/*!
 *  @file   stock_trading_starter_sbi.cpp
 *  @brief  ������X�^�[�g�W�FSBI�p
 *  @date   2017/12/20
 */
#include "stock_trading_starter_sbi.h"

#include "securities_session_sbi.h"
#include "trade_assistant_setting.h"

#include "cipher_aes.h"
#include "random_generator.h"
#include "twitter/twitter_session.h"
#include "utility/utility_datetime.h"

namespace trading
{

class StockTradingStarterSbi::PIMPL
{
private:
    enum eSequence
    {
        SEQ_NONE,   //!< �������ĂȂ�
        SEQ_BUSY,   //!< ������
        SEQ_READY,  //!< ����OK
    };

    //!< �،���ЂƂ̃Z�b�V�����𖳃A�N�Z�X�ňێ��ł��鎞��[�~���b]
    const int64_t m_session_keep_ms;
    //!< �،���ЂƂ̃Z�b�V����
    SecuritiesSessionPtr m_pSecSession;
    //!< twitter�Ƃ̃Z�b�V����(���b�Z�[�W�ʒm�p)
    garnet::TwitterSessionForAuthorPtr m_pTwSession;

    eSequence m_sequence;                                   //!< �V�[�P���X
    eStockInvestmentsType m_last_register_investments;      //!< �Ō�ɊĎ�������o�^������������

    /*!
     *  @brief  �ۗL�����擾
     *  @param  update_func �ۗL�����X�V�֐�
     */
     void GetStockOwned(const UpdateStockHoldingsFunc& update_func)
     {
        m_pSecSession->GetStockOwned([this, update_func]
                                     (bool b_result, const SpotTradingsStockContainer& spot,
                                                     const StockPositionContainer& position,
                                                     const std::wstring& sv_date)
        {
            if (b_result) {
                m_sequence = SEQ_READY;
                update_func(spot, position, sv_date);
            }
            // ���s�����ꍇ��BUSY�̂܂�(�X�^�[�^�[�Ăяo�����őΏ�)
        });
        m_sequence = SEQ_BUSY;
     }
    /*!
     *  @brief  �Ď������R�[�h�o�^
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_type    ��������
     *  @param  init_func           �Ď������������֐�
     *  @param  update_func         �ۗL�����X�V�֐�
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const InitMonitoringBrandFunc& init_func,
                                const UpdateStockHoldingsFunc& update_func)
    {
        // �O��Ɠ���������Ȃ̂œo�^�s�v
        if (m_last_register_investments == investments_type) {
            GetStockOwned(update_func);
            return;
        }
        m_pSecSession->RegisterMonitoringCode(monitoring_code, investments_type,
                                              [this, investments_type,
                                                     init_func,
                                                     update_func]
                                              (bool b_result, const StockBrandContainer& rcv_brand)
        {
            if (b_result && init_func(investments_type, rcv_brand)) {
                GetStockOwned(update_func);
            }
            // ���s�����ꍇ��BUSY�̂܂�(�X�^�[�^�[�Ăяo�����őΏ�)
        });
        m_last_register_investments = investments_type;
        m_sequence = SEQ_BUSY;
    }

public:
    /*!
     *  @param  sec_session �،���ЂƂ̃Z�b�V����
     *  @param  tw_session  twitter�Ƃ̃Z�b�V����
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    PIMPL(const SecuritiesSessionPtr& sec_session,
          const garnet::TwitterSessionForAuthorPtr& tw_session,
          const TradeAssistantSetting& script_mng)
    : m_session_keep_ms(garnet::utility_datetime::ToMiliSecondsFromMinute(script_mng.GetSessionKeepMinute()))
    , m_pSecSession(sec_session)
    , m_pTwSession(tw_session)
    , m_sequence(SEQ_NONE)
    , m_last_register_investments(INVESTMENTS_NONE)
    {
    }

    /*!
     *  @brief  ����������ł��Ă邩
     *  @retval true    ����OK
     */
    bool IsReady() const
    {
        return m_sequence == SEQ_READY;
    }

    /*!
     *  @brief  �J�n����
     *  @param  tickCount           �o�ߎ���[�~���b]
     *  @param  aes_uid
     *  @param  aes_pwd
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_type    ��������
     *  @param  init_func           �Ď������������֐�:
     *  @param  update_func         �ۗL�����X�V�֐�
     *  @retval true                ����
     */
    bool Start(int64_t tickCount,
               const garnet::CipherAES_string& aes_uid,
               const garnet::CipherAES_string& aes_pwd,
               const StockCodeContainer& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitMonitoringBrandFunc& init_func,
               const UpdateStockHoldingsFunc& update_func)
    {
        if (m_sequence != SEQ_NONE && m_sequence != SEQ_READY) {
            return false; // �J�n������
        }

        const int64_t diff_tick = tickCount - m_pSecSession->GetLastAccessTime();
        if (diff_tick > m_session_keep_ms) {
            // SBI�ւ̍ŏI�A�N�Z�X����̌o�ߎ��Ԃ��Z�b�V�����ێ����Ԃ𒴂��Ă����烍�O�C�����s
            std::wstring uid, pwd;
            aes_uid.Decrypt(uid);
            aes_pwd.Decrypt(pwd);
            m_pSecSession->Login(uid, pwd,
                                 [this, monitoring_code, investments_type,
                                  update_func, init_func]
                                  (bool b_result, bool b_login, bool b_important_msg,
                                   const std::wstring& sv_date)
            {
                if (!b_result) {
                    m_pTwSession->Tweet(sv_date, L"���O�C���G���[�B�ً}�����e�i���X����������܂���B");
                } else if (!b_login) {
                    m_pTwSession->Tweet(sv_date, L"���O�C���ł��܂���ł����BID�܂��̓p�X���[�h���Ⴂ�܂��B");
                    // >ToDo< �ē��͂ł���悤�ɂ���(�g���[�h�J�n�{�^�������Amachine�V�[�P���X���Z�b�g)
                } else {
                    if (b_important_msg) {
                        m_pTwSession->Tweet(sv_date, L"���O�C�����܂����BSBI����̏d�v�Ȃ��m�点������܂��B");
                    } else {
                        m_pTwSession->Tweet(sv_date, L"���O�C�����܂���");
                    }
                    // �Ď������o�^
                    RegisterMonitoringCode(monitoring_code,
                                           investments_type,
                                           init_func, update_func);
                }
            });
            m_sequence = SEQ_BUSY;
        } else {
            RegisterMonitoringCode(monitoring_code, investments_type, init_func, update_func);
        }
        return true;
    }
};

/*!
 *  @param  sec_session �،���ЂƂ̃Z�b�V����
 *  @param  tw_session  twitter�Ƃ̃Z�b�V����
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
 */
StockTradingStarterSbi::StockTradingStarterSbi(const SecuritiesSessionPtr& sec_session,
                                               const garnet::TwitterSessionForAuthorPtr& tw_session,
                                               const TradeAssistantSetting& script_mng)
: StockTradingStarter()
, m_pImpl(new PIMPL(sec_session, tw_session, script_mng))
{
}
/*!
 */
StockTradingStarterSbi::~StockTradingStarterSbi()
{
}

/*!
 *  @brief  ����������ł��Ă邩
 */
bool StockTradingStarterSbi::IsReady() const
{
    return m_pImpl->IsReady();
}

/*!
 *  @brief  �J�n����
 *  @param  tickCount           �o�ߎ���[�~���b]
 *  @param  aes_uid
 *  @param  aes_pwd
 *  @param  monitoring_code     �Ď������R�[�h
 *  @param  investments_type    ��������
 *  @param  init_func           �Ď������������֐�
 *  @param  update_func         �ۗL�����X�V�֐�
 */
bool StockTradingStarterSbi::Start(int64_t tickCount,
                                   const garnet::CipherAES_string& aes_uid,
                                   const garnet::CipherAES_string& aes_pwd,
                                   const StockCodeContainer& monitoring_code,
                                   eStockInvestmentsType investments_type,
                                   const InitMonitoringBrandFunc& init_func,
                                   const UpdateStockHoldingsFunc& update_func)
{
    return m_pImpl->Start(tickCount,
                          aes_uid, aes_pwd,
                          monitoring_code, investments_type, init_func, update_func);
}

} // namespace trading
