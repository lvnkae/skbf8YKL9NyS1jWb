/*!
 *  @file   twitter_session_fwd.h
 *  @brief  [common]twitterAPIセッションforward
 *  @date   2018/01/09
 */
#pragma once

#include <memory>

namespace garnet
{

/*!
 *  @brief  twitterAPIセッション共有ポインタ
 */
class TwitterSessionForAuthor;
typedef std::shared_ptr<TwitterSessionForAuthor> TwitterSessionForAuthorPtr;

} // namespace trading
