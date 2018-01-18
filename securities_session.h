/*!
 *  @file   securities_session.h
 *  @brief  �،���ЃT�C�g�Ƃ̃Z�b�V�����Ǘ�
 *  @date   2017/05/05
 */
#pragma once

#include "trade_container.h"
#include "trade_define.h"

#include <functional>
#include <string>
#include <vector>

namespace garnet { struct YYMMDD; }

namespace trading
{

struct StockExecInfoAtOrder;
struct StockOrder;
struct RcvStockValueData;
struct RcvResponseStockOrder;

class SecuritiesSession
{
public:
    typedef std::function<void (bool b_result, bool, bool,
                                const std::wstring& sv_date)> LoginCallback;
    typedef std::function<void (bool b_result,
                                const StockBrandContainer&)> RegisterMonitoringCodeCallback;
    typedef std::function<void (bool b_result,
                                const SpotTradingsStockContainer&,
                                const StockPositionContainer&,
                                const std::wstring& sv_date)> GetStockOwnedCallback;
    typedef std::function<void (bool b_result,
                                const std::vector<RcvStockValueData>&,
                                const std::wstring& sv_date)> UpdateValueDataCallback;
    typedef std::function<void (bool b_result,
                                const std::vector<StockExecInfoAtOrder>&)> UpdateStockExecInfoCallback;
    typedef std::function<void (bool b_result)> UpdateMarginCallback;
    typedef std::function<void (bool b_result,
                                const RcvResponseStockOrder&,
                                const std::wstring& sv_date)> OrderCallback;

    SecuritiesSession();
    virtual ~SecuritiesSession();


    /*!
     *  @breif  ���O�C��
     *  @param  uid
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    virtual void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback) = 0;

    /*!
     *  @brief  �Ď������R�[�h�o�^
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_type    ����������
     *  @param  callback            �R�[���o�b�N
     */
    virtual void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                        eStockInvestmentsType investments_type,
                                        const RegisterMonitoringCodeCallback& callback) = 0;
    /*!
     *  @brief  �ۗL�������擾
     *  @param  callback    �R�[���o�b�N
     */
    virtual void GetStockOwned(const GetStockOwnedCallback& callback) = 0;

    /*!
     *  @brief  �Ď��������i�f�[�^�擾
     *  @param  callback    �R�[���o�b�N
     */
    virtual void UpdateValueData(const UpdateValueDataCallback& callback) = 0;
    /*!
     *  @brief  �����擾�擾
     *  @param  callback    �R�[���o�b�N
     */
    virtual void UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback) = 0;
    /*!
     *  @brief  �]�͎擾
     *  @param  callback    �R�[���o�b�N
     */
    virtual void UpdateMargin(const UpdateMarginCallback& callback) = 0;

    /*!
     *  @brief  ��������
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     *  @note   ��������/�M�p�V�K����
     */
    virtual void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) = 0;
    /*!
     *  @brief  �M�p�ԍϒ���
     *  @param  t_yymmdd    ����
     *  @param  t_value     ���P��
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    virtual void RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                        const StockOrder& order,
                                        const std::wstring& pwd,
                                        const OrderCallback& callback) = 0;
    /*!
     *  @brief  ��������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    virtual void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback) = 0;
    /*!
     *  @brief  �������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    virtual void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback) = 0;


    /*!
     *  @brief  �،���ЃT�C�g�ŏI�A�N�Z�X�����擾
     *  @return �A�N�Z�X����
     */
    virtual int64_t GetLastAccessTime() const = 0;

private:
    SecuritiesSession(const SecuritiesSession&);
    SecuritiesSession(SecuritiesSession&&);
    SecuritiesSession& operator= (const SecuritiesSession&);
};

} // namespace trading
