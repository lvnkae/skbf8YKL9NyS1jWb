/*!
 *  @file   trade_assistant_setting.h
 *  @brief  �g���[�f�B���O�⏕�F�O���ݒ�(�X�N���v�g)
 *  @date   2017/12/09
 *  @note   �X�N���v�g�ւ̃A�N�Z�X����
 *  @note   lua�A�N�Z�T���B������(�\��͂Ȃ���lua�ȊO���g����悤��)
 */
#pragma once

#include "trade_define.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace garnet { struct MMDD; }
class UpdateMessage;

namespace trading
{
struct StockTimeTableUnit;
class StockTradingTactics;

class TradeAssistantSetting
{
public:
    TradeAssistantSetting();
    ~TradeAssistantSetting();

    /*!
     *  @brief  �ݒ�t�@�C���ǂݍ���
     *  @param[out] o_message
     *  @retval true    ����
     */
    bool ReadSetting(UpdateMessage& o_message);

    /*!
     *  @brief  �g���[�h��ʎ擾
     */
    eTradingType GetTradingType() const;
    /*!
     *  @brief  �،���Ў�ʎ擾
     */
    eSecuritiesType GetSecuritiesType() const;
    /*!
     *  @brief  ���A�N�Z�X�Ŏ���T�C�g�Ƃ̃Z�b�V�������ێ��ł��鎞��
     *  @return �ێ�����[��]
     */
    int32_t GetSessionKeepMinute() const;
    /*!
     *  @brief  �ً}���[�h���Ԏ擾
     *  @return ��p����[�b]
     */
    int32_t GetEmergencyCoolSecond() const;
    /*!
     *  @brief  �����i�f�[�^�X�V�Ԋu�擾
     *  @return �X�V�Ԋu[�b]
     */
    int32_t GetStockMonitoringIntervalSecond() const;
    /*!
     *  @brief  ���������X�V�Ԋu�擾
     *  @return �X�V�Ԋu[�b]
     */
    int32_t GetStockExecInfoIntervalSecond() const;
    /*!
     *  @brief  �Ď������ő�o�^���擾
     */
    int32_t GetMaxMonitoringCodeRegister() const;
    /*!
     *  @brief  �Ď��������o�̓f�B���N�g���擾
     */
    std::string GetStockMonitoringLogDir() const;
    /*!
     *  @brief  �����Ď��Ɏg�p����|�[�g�t�H���I�ԍ��擾
     */
    int32_t GetUsePortfolioNumberForMonitoring() const;
    /*!
     *  @brief  �|�[�g�t�H���I�\���`��(�Ď������p)�擾
     */
    int32_t GetPortfolioIndicateForMonitoring() const;
    /*!
     *  @brief  �|�[�g�t�H���I�\���`��(�ۗL�����p)�擾
     */
    int32_t GetPortfolioIndicateForOwned() const;

    /*!
     *  @brief  JPX�̌ŗL�x�Ɠ��f�[�^�\�z
     *  @param[out] o_message
     *  @param[out] o_holidays  �x�Ɠ��f�[�^�i�[��
     *  @retval true    ����
     *  @note   lua�ɃA�N�Z�X����s����const�ɂł��Ȃ�
     */
    bool BuildJPXHoliday(UpdateMessage& o_message, std::vector<garnet::MMDD>& o_holidays);
    /*!
     *  @brief  ������^�C���e�[�u���\�z
     *  @param[out] o_message
     *  @param[out] o_tt        �^�C���e�[�u���i�[��
     *  @retval true    ����
     *  @note   lua�ɃA�N�Z�X����s����const�ɂł��Ȃ�
     */
    bool BuildStockTimeTable(UpdateMessage& o_message, std::vector<StockTimeTableUnit>& o_tt);
    /*!
     *  @brief  ������헪�f�[�^�\�z
     *  @param[out] o_message
     *  @param[out] o_tactics   �헪�f�[�^�i�[��
     *  @param[out] o_link      �R�t�����i�[��
     *  @retval true    ����
     *  @note   lua�ɃA�N�Z�X����s����const�ɂł��Ȃ�
     */
    bool BuildStockTactics(UpdateMessage& o_message,
                           std::unordered_map<int32_t, StockTradingTactics>& o_tactics,
                           std::vector<std::pair<uint32_t, int32_t>>& o_link);

    /*!
     *  @brief  ����X�N���v�g�֐��Ăяo��
     *  @param  func_ref    �֐��Q�ƒl
     *  @param              �ȍ~�X�N���v�g�ɓn������
     *  @return ���茋��
     */
    bool CallJudgeFunction(int32_t func_ref, float64, float64, float64, float64);
    /*!
     *  @brief  �l�擾�X�N���v�g�֐��Ăяo��
     *  @param  func_ref    �֐��Q�ƒl
     *  @param              �ȍ~�X�N���v�g�ɓn������
     *  @return �Ȃ񂩒l
     */
    float64 CallGetValueFunction(int32_t func_ref, float64, float64, float64, float64);

private:
    TradeAssistantSetting(const TradeAssistantSetting&);
    TradeAssistantSetting& operator= (const TradeAssistantSetting&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
