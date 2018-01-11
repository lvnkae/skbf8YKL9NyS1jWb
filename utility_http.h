/*!
 *  @file   utility_http.h
 *  @brief  [common]HTTP関連Utility
 *  @date   2017/12/19
 *  @note   C++REST, utility_string に依存している
 */
#pragma once

#include <string>

namespace web { namespace http { class http_request; } }
namespace web { namespace http { class http_headers; } }
namespace web { namespace http { class cookies_group; } }

namespace garnet
{
namespace utility_http
{

/*!
 *  @brief  送信リクエストのヘッダー共通設定(最小構成)
 *  @param[out] request 送信リクエスト(操作対象)
 *  @note   Content-Lengthはセットされない(SetFormDataで対応)
 */
void SetHttpCommonHeaderSimple(web::http::http_request& request);
/*!
 *  @brief  送信リクエストのヘッダー共通設定(keep-alive版)
 *  @param[in]  url         送信先URL
 *  @param[in]  cookies_gr  クッキー群
 *  @param[in]  referer     リファラ(emptyならセットしない)
 *  @param[out] request     送信リクエスト(操作対象)
 *  @note   Content-Lengthはセットされない(SetFormDataで対応)
 */
void SetHttpCommonHeaderKeepAlive(const std::wstring& url, const web::http::cookies_group& cookies_gr, const std::wstring& referer, web::http::http_request& request);
/*!
 *  @brief  送信リクエストにform dataを設定する
 *  @param[in]  src     設定する文字列(form data)
 *  @param[out] request 送信リクエスト
 *  @note   Content-Lengthの算出及びセットも行う
 */
void SetFormData(const std::wstring& src, web::http::http_request& request);

/*!
 *  @brief  form dataに要素を追加する
 *  @param[in]  param   form dataパラメータ
 *  @param[out] dst     格納先
 */
struct sFormDataParam
{
    const std::wstring m_name;
    const std::wstring m_value;
};
void AddFormDataParamToString(const sFormDataParam& param, std::wstring& dst);
/*!
 *  @brief  form dataに要素を追加する
 *  @param[in]  param_name  パラメータ名
 *  @param[in]  param_value パラメータ値
 *  @param[out] dst         格納先
 */
void AddFormDataParamToString(const std::wstring& param_name, const std::wstring& param_value, std::wstring& dst);

/*!
 *  @brief  URLにitemを追加する
 *  @param[in]  iname   item-name
 *  @param[in]  ivalue  item-value
 *  @param[out] url     結合先
 *  @note   URLと先頭itemは'?'、item間は'&'で繋ぐ
 *  @note   item-nameとvalueは'='で繋ぐ
 */
void AddItemToURL(const std::wstring& iname, const std::wstring& ivalue, std::wstring& url);

/*!
 *  @brief  URLからドメインを得る(切り出す)
 *  @param[in]  url     URL
 *  @param[in]  domain  ドメイン(格納先)
 *  @note  ドメインの大文字/小文字は区別しない
 *  @note  日本語ドメインは考慮してない(動作不定)
 */
void GetDomainFromURL(const std::string& url, std::string& domain);
void GetDomainFromURL(const std::wstring& urlT, std::string& domain);

} // namespace utility_http
} // namespace garnet
