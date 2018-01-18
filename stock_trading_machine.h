/*!
 *  @file   stock_trading_machine.h
 *  @brief  ���������F��
 *  @date   2017/05/05
 */
#pragma once

#include "trading_machine.h"

#include "twitter/twitter_session_fwd.h"

#include <memory>

namespace trading
{

class StockTradingMachine : public TradingMachine
{
public:
    /*!
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param  tw_session  twitter�Ƃ̃Z�b�V����
     */
    StockTradingMachine(const TradeAssistantSetting& script_mng,
                        const garnet::TwitterSessionForAuthorPtr& tw_session);
    /*!
     */
    ~StockTradingMachine();

    /*!
     *  @brief  �g���[�h�J�n�ł��邩�H
     *  @retval true    �J�n�ł���
     */
    bool IsReady() const override;

    /*!
     *  @brief  �g���[�h�J�n
     *  @param  tickCount   �o�ߎ���[�~���b]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(int64_t tickCount,
               const std::wstring& uid,
               const std::wstring& pwd,
               const std::wstring& pwd_sub) override;

    /*!
     *  @brief  Update�֐�
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
     */
    void Update(int64_t tickCount,
                TradeAssistantSetting& script_mng,
                UpdateMessage& o_message) override;

private:
    StockTradingMachine();

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
