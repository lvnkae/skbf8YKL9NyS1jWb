/*!
 *  @file   http_cookies.h
 *  @brief  [common]cookie
 *  @date   2017/12/25
 *  @note   cpprestsdk に依存(namespaceも準拠する)
 */
#pragma once

#include <string>
#include <unordered_map>

namespace web
{
namespace http
{

class http_headers;

/*!
 *  @brief  cookieクラス
 *  @note   1ドメイン分
 */
class cookies
{
private:
    //! キーとパラメータのセット
    std::unordered_map<std::wstring, std::wstring> m_contents;

public:
    /*!
     *  @brief  受信ヘッダの持つcookieを取り出してセット
     *  @param  headers ResponseHeaders
     */
    void Set(const web::http::http_headers& headers);
    /*!
     *  @brief  連結した一つの文字列として取得する
     *  @param[out] dst RequestHeadersのcookieに入れる文字列(格納先)
     *  @note   keyとparamを'='で繋いだobjを';'で区切って連結
     */
    void Get(std::wstring& dst) const;

    cookies()
    : m_contents()
    {}
    ~cookies(){}
};

/*!
 *  @brief  cookie_groupクラス
 *  @note   ドメインをキーに複数のcookieを束ねたもの
 */
class cookies_group
{
private:
    //! ドメインキーとcookiesのセット
    std::unordered_map<std::string, cookies> m_cookies;

public:
    /*!
     *  @brief  http_headersからcookieを取り出してセット
     *  @param  headers ResponseHeaders
     *  @param  url     送信元URL(headersが保持してくれてれば楽だったのに…)
     *  @note   入力URLはutf-8(string)かutf-16(wstring)
     */
    void Set(const web::http::http_headers& headers, const std::string& url);
    void Set(const web::http::http_headers& headers, const std::wstring& url_t);
    /*!
     *  @brief  cookieを連結した一つの文字列として取得する
     *  @param[in]  url 送信先URL
     *  @param[out] dst 格納先
     *  @note   RequestHeadersに設定するcookie文字列を得る
     *  @note   入力URLはutf-8(string)かutf-16(wstring)
     */
    void Get(const std::string& url, std::wstring& dst) const;
    void Get(const std::wstring& url_t, std::wstring& dst) const;

    cookies_group()
    : m_cookies()
    {}
    ~cookies_group(){}
};

} // namespace http
} // namespace cookie
