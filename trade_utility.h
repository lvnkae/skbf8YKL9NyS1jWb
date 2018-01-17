/*!
 *  @file   trade_utility.h
 *  @brief  �g���[�h�֘Autility�֐�
 *  @date   2018/01/05
 */
#pragma once

namespace trading
{
namespace trade_utility
{

/*!
 *  @brief  ���������ꂩ
 *  @param  src
 *  @param  dst
 *  @retval ����Ƃ݂Ȃ�
 *  @note   ���������_��r�Ȃ̂ň�v�͔͈̓`�F�b�N
 */
bool same_value(float64 src, float64 dst);

/*!
 *  @brief  ���s�w�肩
 *  @param  value   �������i
 */
bool is_market_order(float64 value);

/*!
 *  @brief  �������ԍ�(�،���Д��s)�F��
 */
int32_t BlankOrderID();

/*!
 *  @brief  ���������_�L�������F1
 */
int32_t ValueOrder();


} // namespace trade_utility
} // namespace trading
