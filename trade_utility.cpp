/*!
 *  @file   trade_utility.cpp
 *  @brief  �g���[�h�֘Autility�֐�
 *  @date   2018/01/05
 */
#include "trade_utility.h"

namespace trading
{
namespace trade_utility
{

/*!
 *  @brief  ���������ꂩ
 *  @param  src
 *  @param  dst
 */
bool same_value(float64 src, float64 dst)
{
    // �����_���ʂ܂ł͗L�������Ȃ̂ł�艺�ʂŔ���
    const float64 diff = src - dst;
    return (-0.05 < diff && diff < 0.05);
}

/*!
 *  @brief  ���s�w�肩
 *  @param  value   �������i
 */
bool is_market_order(float64 value)
{
    // 0�����Ȃ琬�s���w��(�X�N���v�g�ł�-1�w��)
    return value < 0.0;
}

/*!
 *  @brief  �������ԍ�(�،���Д��s)�F��
 */
int32_t BlankOrderID() { return -1; }
/*!
 *  @brief  ���������_�L�������F1
 */
int32_t ValueOrder() { return 1; }

} // namespace trade_utility
} // namespace trading
