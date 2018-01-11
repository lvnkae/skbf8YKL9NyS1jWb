/*!
 *  @file   securities_session_fwd.h
 *  @brief  証券会社サイトとのセッション管理forward
 *  @date   2018/01/09
 */
#pragma once

#include <memory>


namespace trading
{

/*!
 *  @brief  証券会社サイトとのセッション管理共有ポインタ
 */
class SecuritiesSession;
typedef std::shared_ptr<SecuritiesSession> SecuritiesSessionPtr;

} // namespace trading
