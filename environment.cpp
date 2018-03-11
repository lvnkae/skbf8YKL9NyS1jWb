/*!
*  @file   environment.cpp
*  @brief  環境設定
*  @date   2017/12/11
*/
#include "environment.h"

#include "google/google_api_config.h"
#include "python/python_config.h"
#include "twitter/twitter_config.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include "boost/optional.hpp"

//! 自身の弱参照
std::weak_ptr<Environment> Environment::m_pInstance;

/*!
 *  @brief  インスタンス生成(static)
 *  @return インスタンス共有ポインタ
 */
std::shared_ptr<Environment> Environment::Create()
{
    if (m_pInstance.lock()) {
        std::shared_ptr<Environment> _empty_instance;
        return _empty_instance;
    } else {
        std::shared_ptr<Environment> _instance(new Environment());
        m_pInstance = _instance;

        _instance->initialize();

        return _instance;
    }
}

/*!
 *   @brief  GoogleCalendarAPI設定を得る(static)
 */
const garnet::GoogleCalendarAPIConfigRef Environment::GetGoogleCarendarAPIConfig()
{
    std::shared_ptr<const Environment> p = m_pInstance.lock();
    if (nullptr != p) {
        return p->m_google_calendar_api_config;
    } else {
        return garnet::GoogleCalendarAPIConfigRef();
    }
}

/*!
 *  @brief  python設定を得る(static)
 */
const garnet::python_config_ref Environment::GetPythonConfig()
{
    std::shared_ptr<const Environment> p = m_pInstance.lock();
    if (nullptr != p) {
        return p->m_python_config;
    } else {
        return garnet::python_config_ref();
    }
}

/*!
 *  @brief  twitter設定を得る(static)
 */
const garnet::twitter_config_ref Environment::GetTwitterConfig()
{
    std::shared_ptr<const Environment> p = m_pInstance.lock();
    if (nullptr != p) {
        return p->m_twitter_config;
    } else {
        return garnet::twitter_config_ref();
    }
}

/*!
 *  @brief
 */
Environment::Environment()
: m_python_config()
, m_trading_script()
{
}

/*!
 *  @brief  初期化
 */
void Environment::initialize()
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("trade_assistant.ini", pt);

    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Initialize.GoogleAPI");
        m_google_calendar_api_config.reset(new garnet::GoogleCalendarAPIConfig(str.get()));
    }
    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Initialize.Python");
        m_python_config.reset(new garnet::python_config(str.get()));
    }
    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Initialize.Twitter");
        m_twitter_config.reset(new garnet::twitter_config(str.get()));
    }
    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Script.TradingScript");
        m_trading_script = str.get();
    }
}
