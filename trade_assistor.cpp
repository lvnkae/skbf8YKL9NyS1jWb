    /*!
 *  @file   trade_assistor.cpp
 *  @brief  �g���[�f�B���O�⏕
 *  @note   ���E�בցE�敨�Ȃǂ̔������O���ݒ�ɏ]���Ď��s����N���X
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
        SEQ_ERROR,          //!< �G���[��~
        SEQ_NONE,           //!< �������ĂȂ�
        SEQ_READSETTING,    //!< �ݒ�t�@�C���ǂݍ��ݒ�
        SEQ_COMPSETTING,    //!< �ݒ芮��(����ł����)
    };

    eSequence m_sequence;                       //!< �V�[�P���X
    TradeAssistantSetting m_setting;            //!< �O���ݒ�Ǘ�
    std::unique_ptr<TradingMachine> m_pMachine; //!< �g���[�h�}�V��

    //!< twitter�Ƃ̃Z�b�V����
    std::shared_ptr<garnet::TwitterSessionForAuthor> m_pTwitterSession;

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  ����X�V�����F������
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
            // ������Machine�쐬
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
     *  @brief  �ݒ�t�@�C���ǂݍ��ݎw��
     */
    void ReadSetting()
    {
        m_sequence = SEQ_READSETTING;
    }

    /*!
     *  @brief  �g���[�h�J�n�ł��邩�H
     *  @retval true    �J�n�ł���
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
     *  @brief  �g���[�h�J�n
     *  @param  tickCount   �o�ߎ���[�~���b]
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
     *  @brief  Update�֐�
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
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
 *  @brief  �O���ݒ�ǂݍ���
 */
void TradeAssistor::ReadSetting()
{
    m_pImpl->ReadSetting();
}

/*!
 *  @brief  �g���[�h�J�n�ł��邩�H
 */
bool TradeAssistor::IsReady() const
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
void TradeAssistor::Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub)
{
    m_pImpl->Start(tickCount, uid, pwd, pwd_sub);
}

/*!
 *  @brief  Update�֐�
 *  @param[in]  tickCount   �o�ߎ���[�~���b]
 *  @param[out] o_message   ���b�Z�[�W(�i�[��)
 */
void TradeAssistor::Update(int64_t tickCount, UpdateMessage& o_message)
{
    m_pImpl->Update(tickCount, o_message);
}

} // namespace trading
