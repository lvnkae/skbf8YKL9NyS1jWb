/*!
 *  @file   trade_assistor.h
 *  @brief  �g���[�f�B���O�⏕
 *  @note   ���E�בցE�敨�Ȃǂ̔������O���ݒ�ɏ]���Ď��s����N���X
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
     *  @brief  �O���ݒ�ǂݍ���
     */
    void ReadSetting();

    /*!
     *  @brief  �g���[�h�J�n�ł��邩�H
     *  @retval true    �J�n�ł���
     */
    bool IsReady() const;

    /*!
     *  @brief  �g���[�h�J�n
     *  @param  tickCount   �o�ߎ���[�~���b]
     *  @param  uid
     *  @param  pwd
     *  @param  pwd_sub
     */
    void Start(int64_t tickCount, const std::wstring& uid, const std::wstring& pwd, const std::wstring& pwd_sub);

    /*!
     *  @brief  �����ꎞ��~
     *  @note   ������������O�����甄�����~�߂Ă��܂�
     */
    void Pause();

    /*!
     *  @brief  Update�֐�
     *  @param[in]  tickCount   �o�ߎ���[�~���b]
     *  @param[out] o_message   ���b�Z�[�W(�i�[��)
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
