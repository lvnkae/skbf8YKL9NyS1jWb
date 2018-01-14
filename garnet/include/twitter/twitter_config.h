/*!
 *  @file   twitter_config.h
 *  @brief  [common]twitter設定
 *  @date   2018/01/14
 *  @note   boost に依存してる
 */
#pragma once

#include <string>

namespace garnet
{
/*!
 *  @brief  twitter設定
 *  @note   iniファイルを読み込んで外部に提供
 */
class twitter_config
{
public:
    /*!
     *  @brief  空生成
     */
    twitter_config();
    /*!
     *  @param  iniファイル名
     */
    twitter_config(const std::string& ini_file_name);

    /*!
     *  @brief  ConsumerKeyを得る
     */
    const std::string& GetConsumerKey() const { return m_consumer_key; }
    /*!
     *  @brief  ConsumerSecretを得る
     */
    const std::string& GetConsumerSecret() const { return m_consumer_secret; }
    /*!
     *  @brief  AccessTokenを得る
     */
    const std::string& GetAccessToken() const { return m_access_token; }
    /*!
     *  @brief  AccessTokenSecretを得る
     */
    const std::string& GetAccessTokenSecret() const { return m_access_token_secret; }

    /*!
     *  @brief  最大ツイート文字数を得る
     */
    int32_t GetMaxTweetLetters() const { return m_max_tweet_letters; }

private:
    twitter_config(const twitter_config&);
    twitter_config(twitter_config&&);
    twitter_config& operator= (const twitter_config&);

    /*!
     *  @brief  初期化
     *  @param  iniファイル名
     */
    void initialize(const std::string& ini_file_name);

    std::string m_consumer_key;
    std::string m_consumer_secret;
    std::string m_access_token;
    std::string m_access_token_secret;

    //! 最大ツイート文字数
    int32_t m_max_tweet_letters;
};

} // namespace garnet
