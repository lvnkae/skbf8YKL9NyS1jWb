/*!
 *  @file   twitter_session.h
 *  @brief  [common]twitterAPIセッション
 *  @date   2017/12/31
 *  @note   C++ REST SDK, boost. utility_datetime に依存している
 */
#pragma once

#include "twitter_config_fwd.h"

#include <string>
#include <memory>

namespace garnet
{

namespace twitter
{

/*!
 *  @brief  改行コード文字列を得る
 */
const std::wstring GetNewlineString();

} //  namespace twitter

/*!
 *  @brief  AccessToken/Secretが判明しているauthor専用セッション
 *  @note   ユーザにブラウザ認証を促しtokenを取得するシーケンスは含まれない
 */
class TwitterSessionForAuthor
{
public:
    /*!
     *  @param  config  twitter設定
     */
    TwitterSessionForAuthor(const twitter_config_ref& config);
    /*!
     *  @param  consumer_key
     *  @param  consumer_secret
     *  @param  access_token
     *  @param  access_token_secret
     *  @param  max_tweet_letters   最大ツイート文字数
     */
    TwitterSessionForAuthor(const std::wstring& consumer_key,
                            const std::wstring& consumer_secret,
                            const std::wstring& access_token,
                            const std::wstring& access_token_secret,
                            size_t max_tweet_letters);
    ~TwitterSessionForAuthor();

    /*!
     *  @brief  ツイートする(即時復帰)
     *  @param  date    日時文字列(ASCII)
     *  @param  src     ツイート文字列(utf-16)
     *  @retval 送信成功
     *  @note   280文字以上あったら弾く
     *  @note   ASCIIコードのチェックはしてないので、twitter側で弾かれることもある
     */
    bool Tweet(const std::wstring& date, const std::wstring& src);
    bool Tweet(const std::wstring& src);

private:
    TwitterSessionForAuthor();
    TwitterSessionForAuthor(const TwitterSessionForAuthor&);
    TwitterSessionForAuthor(TwitterSessionForAuthor&&);
    TwitterSessionForAuthor& operator= (const TwitterSessionForAuthor&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace garnet
