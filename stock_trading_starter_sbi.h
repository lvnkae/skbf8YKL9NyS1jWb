/*!
 *  @file   stock_trading_starter_sbi.h
 *  @brief  ������X�^�[�g�W�FSBI�p
 *  @date   2017/12/20
 *  @note   ���O�C�����Ď������o�^���ۗL�����擾 �̏��Ŏ��s
 */
#pragma once

#include "stock_trading_starter.h"
#include "securities_session_fwd.h"

#include "twitter/twitter_session_fwd.h"

#include <memory>

namespace trading
{
class TradeAssistantSetting;

class StockTradingStarterSbi : public StockTradingStarter
{
public:
    /*!
     *  @param  sec_session �،���ЂƂ̃Z�b�V����
     *  @param  tw_session  twitter�Ƃ̃Z�b�V����
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    StockTradingStarterSbi(const SecuritiesSessionPtr& sec_session,
                           const garnet::TwitterSessionForAuthorPtr& tw_session,
                           const TradeAssistantSetting& script_mng);
    /*!
     */
    ~StockTradingStarterSbi();

    /*!
     *  @brief  ����������ł��Ă邩
     *  @retval true    ����OK
     */
    bool IsReady() const override;

    /*!
     *  @brief  �J�n����
     *  @param  tickCount           �o�ߎ���[�~���b]
     *  @param  aes_uid
     *  @param  aes_pwd
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_type    ��������
     *  @param  init_func           �Ď������������֐�
     *  @param  update_func         �ۗL�����X�V�֐�
     *  @retval true                ����
     */
    bool Start(int64_t tickCount,
               const garnet::CipherAES_string& aes_uid,
               const garnet::CipherAES_string& aes_pwd,
               const StockCodeContainer& monitoring_code,
               eStockInvestmentsType investments_type,
               const InitMonitoringBrandFunc& init_func,
               const UpdateStockHoldingsFunc& update_func);

private:
    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace trading
