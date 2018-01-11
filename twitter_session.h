/*!
 *  @file   twitter_session.h
 *  @brief  [common]twitterAPIセッション
 *  @date   2017/12/31
 *  @note   C++ REST SDK, boost. utility_datetime に依存している
 */
#pragma once

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
 *  @note   tokenは設定ファイル(twitter_session.ini)から取得
 */
class TwitterSessionForAuthor
{
public:
    TwitterSessionForAuthor();
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
    TwitterSessionForAuthor(const TwitterSessionForAuthor&);
    TwitterSessionForAuthor& operator= (const TwitterSessionForAuthor&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace garnet
