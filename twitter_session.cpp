/*!
 *  @file   twitter_session.cpp
 *  @brief  [common]twitterAPIセッション
 *  @date   2017/12/31
 */
#include "twitter_session.h"

#include "utility_debug.h"
#include "utility_http.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include "boost/optional.hpp"
#include "cpprest/http_client.h"
#include "cpprest/oauth1.h"
#include <codecvt>

class TwitterSessionForAuthor::PIMPL
{
private:
    std::wstring m_consumer_key;
    std::wstring m_consumer_secret;
    std::wstring m_access_token;
    std::wstring m_access_token_secret;

    size_t m_max_tweet_letters;
    
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

public:
    PIMPL()
    : m_consumer_key()
    , m_consumer_secret()
    , m_access_token()
    , m_access_token_secret()
    , m_max_tweet_letters(0)
    {
        const std::string inifile("twitter_session.ini");
        bool b_exist = false;
        {
            std::ifstream ifs(inifile);
            b_exist = ifs.is_open();
        }
        if (b_exist) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
            boost::property_tree::ptree pt;
            boost::property_tree::read_ini(inifile, pt);

            boost::optional<std::string> str_ck = pt.get_optional<std::string>("OAuth.ConsumerKey");
            m_consumer_key = std::move(utfconv.from_bytes(str_ck.get()));
            boost::optional<std::string> str_cs = pt.get_optional<std::string>("OAuth.ConsumerSecret");
            m_consumer_secret = std::move(utfconv.from_bytes(str_cs.get()));
            boost::optional<std::string> str_at = pt.get_optional<std::string>("OAuth.AccessToken");
            m_access_token = std::move(utfconv.from_bytes(str_at.get()));
            boost::optional<std::string> str_ats = pt.get_optional<std::string>("OAuth.AccessTokenSecret");
            m_access_token_secret = std::move(utfconv.from_bytes(str_ats.get()));

            boost::optional<int32_t> max_letter = pt.get_optional<int32_t>("Setting.MaxTweetLetters");
            m_max_tweet_letters = max_letter.get();
        }
    }

    /*!
     *  @brief  ツイートする
     *  @note   src ツイート文字列(utf-16)
     */
    bool Tweet(const std::wstring& src)
    {
        if (src.size() > m_max_tweet_letters) {
            return false;   // 長すぎる
        }

        web::http::http_request request(web::http::methods::POST);
        utility::SetHttpCommonHeaderSimple(request);
        web::http::oauth1::experimental::oauth1_config oa1_conf(m_consumer_key,
                                                                m_consumer_secret,
                                                                L"https://api.twitter.com/oauth/request_token",
                                                                L"https://api.twitter.com/oauth/authorize",
                                                                L"https://api.twitter.com/oauth/access_token",
                                                                L"http://127.0.0.1",
                                                                web::http::oauth1::experimental::oauth1_methods::hmac_sha1);
        
        oa1_conf.set_token(web::http::oauth1::experimental::oauth1_token(m_access_token, m_access_token_secret));
        web::http::client::http_client_config conf;
        conf.set_oauth1(oa1_conf);
        //
        std::wstring enc_src(std::move(web::uri::encode_data_string(src))); // tweet文字列はURLエンコードしておく
        std::wstring form_data;
        utility::AddFormDataParamToString(L"status", enc_src, form_data);
        utility::SetFormData(form_data, request);
        //
        //utility::string_t str = request.extract_string(true).get();
        web::http::client::http_client http_client(L"https://api.twitter.com/1.1/statuses/update.json", conf);
        http_client.request(request).then([this](web::http::http_response response)
        {
            utility::string_t date_str(response.headers().date());
        });
        
        return true;
    }
};

/*!
 *  @brief
 */
TwitterSessionForAuthor::TwitterSessionForAuthor()
: m_pImpl(new PIMPL())
{
}
/*!
 */
TwitterSessionForAuthor::~TwitterSessionForAuthor()
{
}

/*!
 *  @brief  ツイートする
 *  @src    ツイート文字列
 */
bool TwitterSessionForAuthor::Tweet(const std::wstring& src)
{
    return m_pImpl->Tweet(src);
}
