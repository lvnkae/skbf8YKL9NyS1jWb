/*!
 *  @file   http_cookies.cpp
 *  @brief  [common]cookie
 *  @date   2017/12/25
 */
#include "http_cookies.h"

#include "boost/algorithm/string.hpp"
#include "cpprest/base_uri.h"
#include "cpprest/http_headers.h"
#include "utility_http.h"
#include "utility_string.h"
#include <codecvt>

const wchar_t COOKIE_OBJ_DELIMITER[] = L";";
const wchar_t COOKIE_OBJ_DELIMITER_SUB[] = L",";
const wchar_t COOKIE_PARAM_PIPE[] = L"=";

/*!
 *  @brief  受信ヘッダの持つcookieを取り出してセット
 *  @param  headers ResponseHeaders
 */
void web::http::cookies::Set(const web::http::http_headers& headers)
{
    const std::string key("set-cookie");
    std::for_each(headers.begin(), headers.end(), [&key, this](const std::pair<utility::string_t, utility::string_t>& elem) {
        // キー"set-cookie"を探す(大文字/小文字は区別しない)
        const std::wstring& hkeyT = elem.first;
        std::string hkey;
        garnet::utility_string::ToLower(hkeyT, hkey);
        if (0 == key.compare(hkey)) {
            // セミコロン・カンマ区切りのデータを分割してセットする
            const std::wstring& ck_str = elem.second;
            std::vector<std::wstring> ck_objs;
            {
                std::vector<std::wstring> ck_objs_sub;
                boost::algorithm::split(ck_objs_sub, ck_str, boost::is_any_of(COOKIE_OBJ_DELIMITER_SUB));
                for (const std::wstring& obj: ck_objs_sub) {
                    if (!obj.empty()) {
                        std::vector<std::wstring> ck_objs_work;
                        boost::algorithm::split(ck_objs_work, obj, boost::is_any_of(COOKIE_OBJ_DELIMITER));
                        ck_objs.insert(ck_objs.end(), ck_objs_work.begin(), ck_objs_work.end()); // 連結
                    }
                }
                for (const std::wstring& obj: ck_objs) {
                    std::vector<std::wstring> ck_param;
                    boost::algorithm::split(ck_param, obj, boost::is_any_of(COOKIE_PARAM_PIPE));
                    if (!ck_param.empty()) {
                        std::vector<std::wstring>::const_iterator& it = ck_param.begin();
                        const std::wstring& key = *it++;
                        if (it != ck_param.end()) {
                            if (!it->empty()) {
                                std::wstring& elem = m_contents[key];
                                elem.assign(it->c_str());
                            }
                        }
                    }
                }
            }
        }
    });
}

/*!
 *  @brief  連結した一つの文字列として取得する
 *  @param[out] dst RequestHeadersのcookieに入れる文字列(格納先)
 */
void web::http::cookies::Get(std::wstring& dst) const
{
    for (const auto& obj: m_contents) {
        dst += obj.first + COOKIE_PARAM_PIPE + obj.second + COOKIE_OBJ_DELIMITER;
    }
}


/*!
 *  @brief  受信ヘッダの持つcookieを取り出してセット
 *  @param  headers ResponseHeaders
 *  @param  url     送ってきたURL
 */
void web::http::cookies_group::Set(const web::http::http_headers& headers, const std::string& url)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
    const std::wstring url_t(std::move(utfconv.from_bytes(url)));
    Set(headers, url_t);
}
void web::http::cookies_group::Set(const web::http::http_headers& headers, const std::wstring& url_t)
{
    const std::wstring enc_url(std::move(web::uri::encode_uri(url_t, web::uri::components::host)));
    std::string domain;
    garnet::utility_http::GetDomainFromURL(enc_url, domain);
    web::http::cookies& elem = m_cookies[domain];
    elem.Set(headers);
}

/*!
 *  @brief  cookieを連結した一つの文字列として取得する
 *  @param[in]  url 送信するURL
 *  @param[out] dst RequestHeadersのcookieに入れる文字列(格納先)
 */
void web::http::cookies_group::Get(const std::string& url, std::wstring& dst) const
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
    const std::wstring url_t(std::move(utfconv.from_bytes(url)));
    Get(url_t, dst);
}
void web::http::cookies_group::Get(const std::wstring& url_t, std::wstring& dst) const
{
    const std::wstring enc_url(std::move(web::uri::encode_uri(url_t, web::uri::components::host)));
    std::string domain;
    garnet::utility_http::GetDomainFromURL(enc_url, domain);
    std::unordered_map<std::string, web::http::cookies>::const_iterator it = m_cookies.find(domain);
    if (it != m_cookies.end()) {
        it->second.Get(dst);
    } else {
        dst.swap(std::wstring());
    }
}
