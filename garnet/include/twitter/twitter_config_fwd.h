/*!
 *  @file   twitter_config_fwd.h
 *  @brief  [common]twitterê›íËforward
 *  @date   2018/01/14
 */
#pragma once

#include <memory>

namespace garnet
{
class twitter_config;
typedef std::shared_ptr<const twitter_config> twitter_config_ptr;
typedef std::weak_ptr<const twitter_config> twitter_config_ref;

} // namespace garnet
