/*!
 *  @file   python_config_fwd.h
 *  @brief  [common]pythonê›íËforward
 *  @date   2018/01/14
 */
#pragma once

#include <memory>

namespace garnet
{
class python_config;
typedef std::shared_ptr<const python_config> python_config_ptr;
typedef std::weak_ptr<const python_config> python_config_ref;

} // namespace garnet
