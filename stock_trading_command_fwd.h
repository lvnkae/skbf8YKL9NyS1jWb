/*!
 *  @file   stock_trading_command_fwd.h
 *  @brief  株取引命令forward
 *  @date   2018/01/07
 */
#pragma once

#include <memory>

namespace trading
{

/*!
 *  @brief  取引命令共有ポインタ
 */
class StockTradingCommand;
typedef std::shared_ptr<StockTradingCommand> StockTradingCommandPtr;

} // namespace trading
