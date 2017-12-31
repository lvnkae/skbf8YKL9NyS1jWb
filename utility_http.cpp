/*!
 *  @file   utility_http.cpp
 *  @brief  [common]HTTP関連Utility
 *  @date   2017/12/19
 */
#include "utility_http.h"

#include "http_cookies.h"
#include "utility_string.h"

#include "boost/algorithm/string.hpp"
#include "cpprest/http_headers.h"
#include "cpprest/http_msg.h"
#include <codecvt>
#include <iostream>

namespace utility
{

/*!
 *  @brief  送信リクエストのヘッダー共通設定(最小構成)
 *  @param[out] request 送信リクエスト(操作対象)
 */
void SetHttpCommonHeaderSimple(web::http::http_request& request)
{
    request.headers().add(L"Accept", L"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    request.headers().add(L"Content-Type", L"application/x-www-form-urlencoded; charset=UTF-8");
    request.headers().add(L"User-Agent", L"Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36");
}
/*!
 *  @brief  送信リクエストのヘッダー共通設定(keep-alive版)
 *  @param[in]  url         送信先URL
 *  @param[in]  cookies_gr  クッキー群
 *  @param[in]  referer     リファラ(emptyならセットしない)
 *  @param[out] request     送信リクエスト(操作対象)
 */
void SetHttpCommonHeaderKeepAlive(const std::wstring& url, const web::http::cookies_group& cookies_gr, const std::wstring& referer, web::http::http_request& request)
{
    SetHttpCommonHeaderSimple(request);
    request.headers().add(L"Connection", L"keep-alive");
    std::wstring cookie_str;
    cookies_gr.Get(url, cookie_str);
    if (!cookie_str.empty()) {
        request.headers().add(L"Cookie", cookie_str);
    }
    if (!referer.empty()) {
        request.headers().add(L"Referer", referer);
    }
}

/*!
 *  @brief  送信リクエストにform dataを設定する
 *  @param[in]  src     設定する文字列(2byte文字列)
 *  @param[out] request 送信リクエスト
 */
void SetFormData(const std::wstring& src, web::http::http_request& request)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
    std::string body_u8(cv.to_bytes(src));
    const size_t body_size = body_u8.length();
    request.headers().add(L"Content-Length", std::to_wstring(body_size));
    request.set_body(src);
}

/*!
 *  @brief  form dataに要素を追加する
 *  @param[in]  param_name  パラメータ名
 *  @param[in]  param_value パラメータ値
 *  @param[out] dst         格納先
 */
void AddFormDataParamToString(const std::wstring& param_name, const std::wstring& param_value, std::wstring& dst)
{
    if (dst.empty()) {
        dst = param_name + L"=" + param_value;
    } else {
        dst += L"&" + param_name + L"=" + param_value;
    }
}
/*!
 *  @brief  form dataに要素を追加する
 *  @param[in]  param   form dataパラメータ
 *  @param[out] dst     格納先
 */
void AddFormDataParamToString(const sFormDataParam& param, std::wstring& dst)
{
    AddFormDataParamToString(param.m_name, param.m_value, dst);
}

/*!
 *  @brief  URLにitemを追加する
 *  @param[in]  iname   item-name
 *  @param[in]  ivalue  item-value
 *  @param[out] url     結合先
 *  @note   URLと先頭itemは'?'、item間は'&'で繋ぐ
 *  @note   item-nameとvalueは'='で繋ぐ
 */
void AddItemToURL(const std::wstring& iname, const std::wstring& ivalue, std::wstring& url)
{
    if (std::wstring::npos == url.find(L'?')) {
        url += L'?' + iname + L"=" + ivalue;
    } else {
        url += L'&' + iname + L"=" + ivalue;
    }
}

/*!
 *  @brief  URLからドメインを得る(切り出す)
 *  @param[in]  url     URL
 *  @param[in]  domain  ドメイン(格納先)
 *  @note  ドメインの大文字/小文字は区別しない
 *  @note  日本語ドメインは考慮してない(動作不定)
 */
template<typename T>
void GetDomainFromURCoreL(const T& url, std::string& domain)
{
    utility::ToLower(url, domain);
    std::vector<std::string> url_split;
    boost::algorithm::split(url_split, domain, boost::is_any_of("/"));
    const size_t len = url_split.size();
    if (len < 2) {
        domain.swap(std::string());
    } else {
        domain.swap(url_split[2]); // [http:][][domain][etc]に分割される
    }
}
void GetDomainFromURL(const std::string& url, std::string& domain) { GetDomainFromURCoreL(url, domain); }
void GetDomainFromURL(const std::wstring& urlT, std::string& domain) { GetDomainFromURCoreL(urlT, domain); }

} // namespace utility
