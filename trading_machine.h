/*!
 *  @file   trading_machine.h
 *  @brief  ���������F�x�[�X
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
     *  @brief  �g���[�h�J�n�ł��邩�H
     *  @retval true    �J�n�ł���
     */
    virtual bool IsReady() const { return false; }

    /*!
     *  @brief  �g���[�h�J�n
     *  @param  tickCount   �o�ߎ���[�~���b]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    virtual void Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub) = 0;

    /*!
     *  @brief  �����ꎞ��~
     */
    virtual void Pause() = 0;

    /*!
     *  @brief  ���O�o��
     */
    virtual void OutputLog() = 0;

    /*!
     *  @brief  Update�֐�
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[in]  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
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
