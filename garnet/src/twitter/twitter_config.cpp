/*!
*  @file   twitter_config.cpp
*  @brief  [common]twitter設定
*  @date   2017/12/11
*/
#include "twitter_config.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include "boost/optional.hpp"

#include <fstream>

namespace garnet
{

/*!
 *  @brief  空生成
 */
twitter_config::twitter_config()
: m_consumer_key()
, m_consumer_secret()
, m_access_token()
, m_access_token_secret()
{
}

/*!
 *  @param  iniファイル名
 */
twitter_config::twitter_config(const std::string& ini_file_name)
: m_consumer_key()
, m_consumer_secret()
, m_access_token()
, m_access_token_secret()
{
    initialize(ini_file_name);
}

/*!
 *  @brief  初期化
 *  @param  iniファイル名
 */
void twitter_config::initialize(const std::string& ini_file_name)
{
    std::ifstream ifs(ini_file_name);
    if (!ifs.is_open()) {
        return; // iniファイルがない
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(ini_file_name, pt);

    boost::optional<std::string> consumer_key
        = pt.get_optional<std::string>("OAuth.ConsumerKey");
    m_consumer_key = consumer_key.get();
    boost::optional<std::string> consumer_secret
        = pt.get_optional<std::string>("OAuth.ConsumerSecret");
    m_consumer_secret = consumer_secret.get();
    boost::optional<std::string> access_token
        = pt.get_optional<std::string>("OAuth.AccessToken");
    m_access_token = access_token.get();
    boost::optional<std::string> access_token_secret
        = pt.get_optional<std::string>("OAuth.AccessTokenSecret");
    m_access_token_secret = access_token_secret.get();
    boost::optional<int32_t> max_tweet_letters
        = pt.get_optional<int32_t>("Setting.MaxTweetLetters");
    m_max_tweet_letters = max_tweet_letters.get();
}

} // namespace garnet
