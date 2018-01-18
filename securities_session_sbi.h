/*!
 *  @file   securities_session_sbi.h
 *  @brief  SBI�،��T�C�g�Ƃ̃Z�b�V�����Ǘ�
 *  @date   2017/05/05
 */
#pragma once

#include "securities_session.h"

#include <memory>

namespace trading
{

struct StockOrder;
class TradeAssistantSetting;

class SecuritiesSessionSbi : public SecuritiesSession
{
public:
    /*!
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    SecuritiesSessionSbi(const TradeAssistantSetting& script_mng);
    /*!
     */
    ~SecuritiesSessionSbi();

    /*!
     *  @breif  ���O�C��
     *  @param  uid
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     *  @note   mobile��PC�̏��Ƀ��O�C��
     */
    void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback) override;

    /*!
     *  @brief  �Ď������R�[�h�o�^
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_type    ����������
     *  @param  callback            �R�[���o�b�N
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const RegisterMonitoringCodeCallback& callback) override;
    /*!
     *  @brief  �ۗL�������擾
     *  @param  callback    �R�[���o�b�N
     */
    void GetStockOwned(const GetStockOwnedCallback& callback) override;

    /*!
     *  @brief  �Ď��������i�f�[�^�擾
     *  @param  callback    �R�[���o�b�N
     */
    void UpdateValueData(const UpdateValueDataCallback& callback) override;
    /*!
     *  @brief  �����擾�擾
     *  @param  callback    �R�[���o�b�N
     */
    void UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback) override;
    /*!
     *  @brief  �]�͎擾
     *  @param  callback    �R�[���o�b�N
     */
    void UpdateMargin(const UpdateMarginCallback& callback) override;

    /*!
     *  @brief  ��������
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     *  @note   ��������/�M�p�V�K����
     */
    void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) override;
    /*!
     *  @brief  �M�p�ԍϒ���
     *  @param  t_yymmdd    ����
     *  @param  t_value     ���P��
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                const StockOrder& order,
                                const std::wstring& pwd,
                                const OrderCallback& callback) override;
    /*!
     *  @brief  ��������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) override;
    /*!
     *  @brief  �������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback) override;


    /*!
     *  @brief  �،���ЃT�C�g�ŏI�A�N�Z�X�����擾
     *  @return �A�N�Z�X����(tickCount)
     *  @note   mobile��PC�ł��Â�����Ԃ�
     */
    int64_t GetLastAccessTime() const override;

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
