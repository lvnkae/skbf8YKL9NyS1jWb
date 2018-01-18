/*!
 *  @file   stock_holdings_keeper.h
 *  @brief  ���ۗL�����Ǘ�
 *  @date   2018/01/09
 */
#pragma once

#include "stock_trading_command_fwd.h"
#include "trade_container.h"

#include <memory>
#include <vector>

namespace garnet { struct YYMMDD; }
namespace trading
{
class StockCode;
struct StockOrder;
struct StockExecInfoAtOrder;

//! �����ςݐM�p�ԍϔ�������<�����ԍ�(�\���p), ��������>
typedef std::unordered_map<int32_t, StockTradingCommandPtr> ServerRepLevOrder;

/*!
 *  @brief  �ۗL�����Ǘ��N���X
 *  @note   �����ۗL���E�M�p���ʂ̊Ǘ�
 */
class StockHoldingsKeeper
{
public:
    StockHoldingsKeeper();
    ~StockHoldingsKeeper();

    /*!
     *  @brief  �ۗL�����X�V
     *  @param  spot        �����ۗL��
     *  @param  position    �M�p�ۗL��(����)
     */
    void UpdateHoldings(const SpotTradingsStockContainer& spot,
                        const StockPositionContainer& position);
    /*!
     *  @brief  ���������X�V
     *  @param  rcv_info        ��M���������(1�����t��)
     *  @param  diff_info       �����(�O��Ƃ̍���)
     *  @param  sv_rep_order    �����(����)�ƑΉ����锭���ςݐM�p�ԍϔ�������
     */
    void UpdateExecInfo(const std::vector<StockExecInfoAtOrder>& rcv_info,
                        const std::vector<StockExecInfoAtOrder>& diff_info,
                        const ServerRepLevOrder& sv_rep_order);

    /*!
     *  @brief  ���������̍����𓾂�
     *  @param[in]  rcv_info    ��M���������(1�����t��)
     *  @param[out] diff_info   �O��Ƃ̍���(�i�[��)    
     */
    void GetExecInfoDiff(const std::vector<StockExecInfoAtOrder>& rcv_info,
                         std::vector<StockExecInfoAtOrder>& diff_info);

    /*!
     *  @brief  �����`�F�b�N
     *  @param  code    �����R�[�h
     *  @param  number  �K�v����
     *  @retval true    �w�������number���ȏ㌻���ۗL���Ă���
     */
    bool CheckSpotTradingStock(const StockCode& code, int32_t number) const;
    /*!
     *  @brief  �����ۗL�����擾
     *  @param  code    �����R�[�h
     *  @retval 0   �ۗL���ĂȂ�
     */
    int32_t GetSpotTradingStockNumber(const StockCode& code) const;


    /*!
     *  @brief  ���ʃ`�F�b�NA
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     *  @param  number  �K�v����
     *  @retval true    �w�茚��/���P���̃|�W�����݁A��������number�ȏ�ł���
     */
    bool CheckPosition(const StockCode& code,
                       const garnet::YYMMDD& date, float64 value, bool b_sell,
                       int32_t number) const;
    /*!
     *  @brief  ���ʃ`�F�b�NB
     *  @param  code    �����R�[�h
     *  @param  b_sell  �����t���O
     *  @param  number  �K�v����
     *  @retval true    �w����������vnumber���ȏ�|�W���Ă�
     *                  (b_sell���^�Ȃ甄���A�U�Ȃ甃���ʂŃ`�F�b�N��������)
     */
    bool CheckPosition(const StockCode& code, bool b_sell, int32_t number) const;
    /*!
     *  @brief  ���ʃ`�F�b�NC
     *  @param  code    �����R�[�h
     *  @param  pos_id  ����ID�Q
     *  @return true    ���ʂ���ȏ�c���Ă�
     */
    bool CheckPosition(const StockCode& code, const std::vector<int32_t>& pos_id);
    /*!
     *  @brief  �M�p�ۗL�����擾
     *  @param  code    �����R�[�h
     *  @param  date    ����
     *  @param  value   ���P��
     *  @param  b_sell  �����t���O
     *  @retval 0   �ۗL���ĂȂ�
     */
    int32_t GetPositionNumber(const StockCode& code,
                              const garnet::YYMMDD& date, float64 value, bool b_sell) const;
    /*!
     *  @brief  �M�p�ۗL���擾
     *  @param  code    �����R�[�h
     *  @param  b_sell  �����t���O
     */
    std::vector<StockPosition> GetPosition(const StockCode& code, bool b_sell) const;
    /*!
     *  @brief  �M�p�ۗL���ŗLID�𓾂�
     *  @param[in]  user_order_id   �����ԍ�(�\���p)
     *  @param[out] dst             �i�[��
     */
    void GetPositionID(int32_t user_order_id, std::vector<int32_t>& dst) const;

private:
    StockHoldingsKeeper(const StockHoldingsKeeper&);
    StockHoldingsKeeper(StockHoldingsKeeper&&);
    StockHoldingsKeeper& operator= (const StockHoldingsKeeper&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};


} // namespace trading
