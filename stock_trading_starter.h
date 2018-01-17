/*!
 *  @file   stock_trading_starter.h
 *  @brief  ������X�^�[�g�W
 *  @date   2017/12/20
 */
#pragma once

#include "trade_container.h"
#include "trade_define.h"

#include <functional>
#include <string>

namespace garnet { class CipherAES_string; }

namespace trading
{
class StockTradingStarter
{
public:
    typedef std::function<bool(eStockInvestmentsType,
                               const StockBrandContainer&)> InitMonitoringBrandFunc;
    typedef std::function<void(const SpotTradingsStockContainer&,
                               const StockPositionContainer&,
                               const std::wstring& sv_date)> UpdateStockHoldingsFunc;

    StockTradingStarter();
    ~StockTradingStarter();

    /*!
     *  @brief  ����������ł��Ă邩
     *  @retval true    ����OK
     */
    virtual bool IsReady() const = 0;

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
    virtual bool Start(int64_t tickCount,
                       const garnet::CipherAES_string& aes_uid,
                       const garnet::CipherAES_string& aes_pwd,
                       const StockCodeContainer& monitoring_code,
                       eStockInvestmentsType investments_type,
                       const InitMonitoringBrandFunc& init_func,
                       const UpdateStockHoldingsFunc& update_func) = 0;

private:
    StockTradingStarter(const StockTradingStarter&);
    StockTradingStarter(StockTradingStarter&&);
    StockTradingStarter& operator= (const StockTradingStarter&);
};

} // namespace trading
