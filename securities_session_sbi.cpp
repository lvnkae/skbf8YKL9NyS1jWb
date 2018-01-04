/*!
 *  @file   securities_session_sbi.cpp
 *  @brief  SBI証券サイトとのセッション管理
 *  @date   2017/05/05
 */
#include "securities_session_sbi.h"

#include "environment.h"
#include "stock_portfolio.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"

#include "http_cookies.h"
#include "utility_datetime.h"
#include "utility_debug.h"
#include "utility_http.h"
#include "utility_python.h"
#include "utility_string.h"

#include "cpprest/http_client.h"
#include "cpprest/filestream.h"

#include <codecvt>
#include <mutex>


namespace trading
{

#include "securities_session_sbi_util.hpp"

class SecuritiesSessionSbi::PIMPL
{
private:
    std::recursive_mutex m_mtx;             //!< 排他制御子
    boost::python::api::object m_python;    //!< pythonスクリプトオブジェクト
    web::http::cookies_group m_cookies_gr;  //!< SBI用cookie群
    int64_t m_last_access_tick_mb;          //!< SBI(mobile)へ最後にアクセスした時刻(tickCount)
    int64_t m_last_access_tick_pc;          //!< SBI(PC)へ最後にアクセスした時刻(tickCount)

    const size_t m_max_portfolio_entry;     //!< ポートフォリオ最大登録数
    const int32_t m_use_portfolio_number;   //!< 使用するポートフォリオ番号

    typedef std::function<std::wstring (const StockOrder&, const std::wstring&, int64_t, const std::wstring&, web::http::http_request&)> PreOrderConfirm;
    typedef std::function<std::wstring (const StockOrder&, int64_t, const std::wstring&, web::http::http_request&)> PreOrderExecute;

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

public:
    /*!
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const TradeAssistantSetting& script_mng)
    /* html解析用のpythonブジェクト生成 */
    : m_python(std::move(PreparePythonScript(Environment::GetPythonHome(), "html_parser_sbi.py")))
    , m_cookies_gr()
    , m_last_access_tick_mb(0)
    , m_last_access_tick_pc(0)
    , m_max_portfolio_entry(script_mng.GetMaxPortfolioEntry())
    , m_use_portfolio_number(script_mng.GetUsePortfolioNumber())
    {
    }

    /*!
     *  @breif  ログイン
     *  @param  uid
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback)
    {
        // mobileサイトログイン
        web::http::http_request request(web::http::methods::POST);
        utility::SetHttpCommonHeaderSimple(request);
        std::wstring form_data;
        utility::AddFormDataParamToString(L"username", uid, form_data);
        utility::AddFormDataParamToString(PARAM_NAME_PASSWORD, pwd, form_data);
        utility::SetFormData(form_data, request);
        const std::wstring url(URL_BK_LOGIN);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, uid, pwd, url, callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, uid, pwd, callback](size_t bytesRead)
            {
                const int INX_RESULT = 0;       // responseLoginMobile実行成否
                const int INX_LOGIN = 1;        // ログイン成否
                const boost::python::tuple t
                    = boost::python::extract<boost::python::tuple>(m_python.attr("responseLoginMobile")(inStringBuffer.collection()));
                bool b_result = boost::python::extract<bool>(t[INX_RESULT]);
                bool b_login_result = boost::python::extract<bool>(t[INX_LOGIN]);
                if (!b_result || !b_login_result) {
                    callback(b_result, b_login_result, false, date_str);
                    return;
                }

                // PCサイトログイン
                web::http::http_request request(web::http::methods::POST);
                utility::SetHttpCommonHeaderSimple(request);
                const utility::sFormDataParam LOGIN_MAIN[] = {
                    { L"JS_FLAG",           L"1" },
                    { L"BW_FLG" ,           L"chrome,NaN" },
                    { L"_ControlID",        L"WPLETlgR001Control" },
                    { L"_DataStoreID",      L"DSWPLETlgR001Control" },
                    { L"_PageID",           L"WPLETlgR001Rlgn10" },
                    { L"_ActionID",         L"loginPortfolio" },
                    { L"getFlg",            L"on" },
                    { L"allPrmFlg",         L"" },
                    { L"_ReturnPageInfo",   L"" },
                };
                std::wstring form_data;
                for (uint32_t inx = 0; inx < sizeof(LOGIN_MAIN)/sizeof(utility::sFormDataParam); inx++) {
                    utility::AddFormDataParamToString(LOGIN_MAIN[inx], form_data);
                }
                utility::AddFormDataParamToString(L"user_id", uid, form_data);
                utility::AddFormDataParamToString(L"user_password", pwd, form_data);
                utility::SetFormData(form_data, request);
                const std::wstring url(URL_MAIN_SBI_MAIN);
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_pc = utility::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    utility::string_t date_str(response.headers().date());
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, callback](size_t bytesRead)
                    {
                        const int INX_RESULT = 0;       // responseLoginPC実行成否
                        const int INX_LOGIN = 1;        // ログイン成否
                        const int INX_IMPORT_MSG = 2;   // 重要なお知らせの有無
                        const boost::python::tuple t
                            = boost::python::extract<boost::python::tuple>(m_python.attr("responseLoginPC")(inStringBuffer.collection()));
                        callback(boost::python::extract<bool>(t[INX_RESULT]),
                                 boost::python::extract<bool>(t[INX_LOGIN]),
                                 boost::python::extract<bool>(t[INX_IMPORT_MSG]),
                                 date_str);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  ポートフォリオ作成
     *  @param  monitoring_code     監視銘柄
     *  @param  investments_code    株取引所種別
     *  @param  callback            コールバック
     */
    void CreatePortfolio(const std::unordered_set<uint32_t>& monitoring_code,
                         eStockInvestmentsType investments_type,
                         const CreatePortfolioCallback& callback)
    {
        // 登録確認(regist_id取得)
        const std::wstring url(std::wstring(URL_BK_BASE) + URL_BK_STOCKENTRYCONFIRM);
        web::http::http_request request(web::http::methods::POST);
        utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);
        std::wstring form_data;
        BuildDummyPortfolioForFormData(m_use_portfolio_number, m_max_portfolio_entry, form_data);
        utility::SetFormData(form_data, request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this,
                                           monitoring_code,
                                           investments_type,
                                           url,
                                           callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this,
                                                                     inStringBuffer,
                                                                     monitoring_code,
                                                                     investments_type,
                                                                     callback](size_t bytesRead)
            {
                const int64_t regist_id = boost::python::extract<int64_t>(m_python.attr("getPortfolioRegistID")(inStringBuffer.collection()));
                if (regist_id < 0) {
                    callback(false, std::unordered_map<uint32_t, std::wstring>());
                    return;
                }

                // 登録実行
                const std::wstring& url(std::wstring(URL_BK_BASE) + URL_BK_STOCKENTRYEXECUTE);
                web::http::http_request request(web::http::methods::POST);
                utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(URL_BK_BASE)+URL_BK_STOCKENTRYCONFIRM, request);
                std::wstring form_data;
                BuildPortfolioForFormData(m_use_portfolio_number, m_max_portfolio_entry, monitoring_code, investments_type, regist_id, form_data);
                utility::SetFormData(form_data, request);
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_mb = utility::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
                    {
                        std::unordered_map<uint32_t, std::wstring> rcv_portfolio;
                        const std::string& html_u8 = inStringBuffer.collection();
                        const boost::python::list rcv_data = boost::python::extract<boost::python::list>(m_python.attr("getPortfolioMobile")(html_u8));
                        const auto len = boost::python::len(rcv_data);
                        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
                        rcv_portfolio.reserve(len);
                        for (auto inx = 0; inx < len; inx++) {
                            const auto elem(std::move(rcv_data[inx]));
                            const std::string u8name(std::move(boost::python::extract<std::string>(elem[0])));
                            const std::wstring u16name(std::move(utfconv.from_bytes(u8name)));
                            const uint32_t code = boost::python::extract<uint32_t>(elem[1]);
                            rcv_portfolio.emplace(code, u16name);
                        }
                        callback(true, rcv_portfolio);
                    });
                });
            });
        });
    }
    /*!
     *  @brief  ポートフォリオ転送
     *  @param  callback    コールバック
     */
    void TransmitPortfolio(const TransmitPortfolioCallback& callback)
    {
        // PF転送入り口取得(site0へのログイン)
        // site0はログイン機能が非公開だが、「PF転送入り口」にアクセスすることでログインされる
        const std::wstring url(std::wstring(URL_MAIN_SBI_MAIN) + URL_MAIN_SBI_TRANS_PF_LOGIN);
        web::http::http_request request(web::http::methods::GET);
        utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), URL_MAIN_SBI_TRANS_PF_CHECK); // ここでsite0に移動…
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
            {
                bool b_success = m_python.attr("responseGetEntranceOfPortfolioTransmission")(inStringBuffer.collection());
                if (!b_success) {
                    callback(false);
                }

                // mb→PC転送
                const std::wstring url(URL_MAIN_SBI_TRANS_PF_EXEC);
                web::http::http_request request(web::http::methods::POST);
                utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_TRANS_PF_CHECK, request);
                const utility::sFormDataParam PF_COPY[] = {
                    { L"copyRequestNumber",         L""     },
                    { L"sender_tool_from" ,         L"5"    },  // 5番はmobileサイト
                    { L"receiver_tool_to",          L"1"    },  // 1番はPCサイト
                    { L"select_add_replace_tool_01",L"1_2"  },  // '1_1'追加 '1_2'上書き
                    { L"receivertoolListCnt",       L"4"    },  // 以下固定値っぽい
                    { L"selectReceivertoolCnt",     L"1"    },
                    { L"toolCode0",                 L"1"    },
                    { L"disabled0",                 L"false"},
                    { L"selected0",                 L"true" },
                    { L"dispMsg0",                  L""     },
                    { L"canNotAdd0",                L"false"},
                    { L"count0",                    L""     },
                    { L"radioName0", L"select_add_replace_tool_01" },
                };
                std::wstring form_data;
                for (uint32_t inx = 0; inx < sizeof(PF_COPY)/sizeof(utility::sFormDataParam); inx++) {
                    utility::AddFormDataParamToString(PF_COPY[inx], form_data);
                }
                utility::SetFormData(form_data, request);
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_pc = utility::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
                    {
                        const bool b_success = m_python.attr("responseReqPortfolioTransmission")(inStringBuffer.collection());
                        callback(b_success);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  価格データ更新
     *  @param  callback    コールバック
     */
    void UpdateValueData(const UpdateValueDataCallback& callback)
    {
        const std::wstring url(std::wstring(URL_MAIN_SBI_MAIN) + URL_MAIN_SBI_PORTFOLIO);
        web::http::http_request request(web::http::methods::GET);
        utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                std::vector<RcvStockValueData> rcv_valuedata;
                const std::string& html_sjis = inStringBuffer.collection();
                const boost::python::list rcv_data = boost::python::extract<boost::python::list>(m_python.attr("getPortfolioPC")(html_sjis));
                const auto len = boost::python::len(rcv_data);
                rcv_valuedata.reserve(len);
                for (auto inx = 0; inx < len; inx++) {
                    auto elem = rcv_data[inx];
                    RcvStockValueData rcv_vunit;
                    rcv_vunit.m_code = boost::python::extract<uint32_t>(elem[0]);
                    rcv_vunit.m_value = boost::python::extract<float64>(elem[1]);
                    rcv_vunit.m_open = boost::python::extract<float64>(elem[2]);
                    rcv_vunit.m_high = boost::python::extract<float64>(elem[3]);
                    rcv_vunit.m_low = boost::python::extract<float64>(elem[4]);
                    rcv_vunit.m_close = boost::python::extract<float64>(elem[5]);
                    rcv_vunit.m_number = boost::python::extract<int64_t>(elem[6]);
                    rcv_valuedata.push_back(rcv_vunit);
                }
                const bool b_sucess = !rcv_valuedata.empty();
                callback(b_sucess, date_str, rcv_valuedata);
            });
        });
    }


    /*!
     *  @brief  汎用株注文
     *  @param  order           注文情報
     *  @param  pwd
     *  @param  callback        コールバック
     *  @param  input_url       注文入力URL
     *  @param  pre_confirm     注文確認前処理
     *  @param  pre_execute     注文発行前処理
     */
    void StockOrderExecute(const StockOrder& order,
                           const std::wstring& pwd,
                           const OrderCallback& callback,
                           const std::wstring& input_url,
                           const PreOrderConfirm& pre_confirm,
                           const PreOrderExecute& pre_execute)
    {
        if (!order.IsValid()) {
            callback(false, RcvResponseStockOrder(), std::wstring());    // 不正注文(error)
            return;
        }
        // 注文入力 ※regist_id取得
        web::http::http_request request(web::http::methods::GET);
        utility::SetHttpCommonHeaderKeepAlive(input_url, m_cookies_gr, std::wstring(), request);
        web::http::client::http_client http_client(input_url);
        http_client.request(request).then([this, input_url, order, pwd, callback,
                                           pre_confirm, pre_execute](web::http::http_response response)
        {
            m_last_access_tick_mb = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), input_url);
            utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str,
                                                                    input_url, order, pwd, callback,
                                                                    pre_confirm, pre_execute](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                const int32_t i_order_type = static_cast<int32_t>(order.m_type);
                const int64_t regist_id
                    = boost::python::extract<int64_t>(m_python.attr("getStockOrderRegistID")(inStringBuffer.collection(), i_order_type));
                if (regist_id < 0) {
                    callback(false, RcvResponseStockOrder(), date_str);
                    return;
                }

                // 注文確認 ※regist_id取得
                web::http::http_request request(web::http::methods::POST);
                std::wstring cf_url(std::move(pre_confirm(order, pwd, regist_id, input_url, request)));
                //
                web::http::client::http_client http_client(cf_url);
                http_client.request(request).then([this, cf_url, order, callback,
                                                   pre_execute](web::http::http_response response)
                {
                    m_last_access_tick_mb = utility::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), cf_url);
                    utility::string_t date_str(response.headers().date());
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str,
                                                                             cf_url, order, callback,
                                                                             pre_execute](size_t bytesRead)
                    {
                        // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                        std::lock_guard<std::recursive_mutex> lock(m_mtx);

                        const int32_t i_order_type = static_cast<int32_t>(order.m_type);
                        const int64_t regist_id
                            = boost::python::extract<int64_t>(m_python.attr("getStockOrderConfirmRegistID")(inStringBuffer.collection(), i_order_type));
                        if (regist_id < 0) {
                            callback(false, RcvResponseStockOrder(), date_str);
                            return;
                        }

                        // 注文発行
                        web::http::http_request request(web::http::methods::POST);
                        std::wstring ex_url(std::move(pre_execute(order, regist_id, cf_url, request)));
                        //
                        web::http::client::http_client http_client(ex_url);
                        http_client.request(request).then([this, ex_url, order, callback](web::http::http_response response)
                        {
                            m_last_access_tick_mb = utility::GetTickCountGeneral();
                            m_cookies_gr.Set(response.headers(), ex_url);
                            utility::string_t date_str(response.headers().date());
                            concurrency::streams::istream bodyStream = response.body();
                            concurrency::streams::container_buffer<std::string> inStringBuffer;
                            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, callback](size_t bytesRead)
                            {
                                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                                std::lock_guard<std::recursive_mutex> lock(m_mtx);
                                //
                                const boost::python::tuple t
                                    = boost::python::extract<boost::python::tuple>(m_python.attr("responseStockOrderExec")(inStringBuffer.collection()));
                                RcvResponseStockOrder rcv_order;
                                const bool b_result = ToRcvResponseStockOrderFromResult_responseStockOrderExec(t, rcv_order);
                                callback(b_result, rcv_order, date_str);
                            });
                        });
                    });
                });
            });
        });
    }

    /*!
     *  @brief  新規売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void FreshOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
    {
        // 注文入力 ※regist_id取得
        std::wstring url(std::move(BuildOrderURL(order.m_b_leverage, URL_BK_ORDER[order.m_type][OSTEP_INPUT])));
        utility::AddItemToURL(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), url);
        utility::AddItemToURL(PARAM_NAME_ORDER_INVESTIMENTS, GetSbiInvestimentsCode(order.m_investiments), url);

        // 注文確認準備function
        const auto pre_confirm = [this](const StockOrder& order,
                                        const std::wstring& pass,
                                        int64_t regist_id,
                                        const std::wstring& input_url,
                                        web::http::http_request& request)->std::wstring
        {
            std::wstring cf_url(std::move(BuildOrderURL(order.m_b_leverage, URL_BK_ORDER[order.m_type][OSTEP_CONFIRM])));
            utility::SetHttpCommonHeaderKeepAlive(cf_url, m_cookies_gr, input_url, request);
            BuildFreshOrderFormData(order, pass, regist_id, request);
            return cf_url;
        };
        // 注文発行準備function
        const auto pre_execute = [this](const StockOrder& order,
                                        int64_t regist_id,
                                        const std::wstring& cf_url,
                                        web::http::http_request& request)->std::wstring
        {
            std::wstring ex_url(std::move(BuildOrderURL(order.m_b_leverage, URL_BK_ORDER[order.m_type][OSTEP_EXECUTE])));
            utility::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildFreshOrderFormData(order, std::wstring(), regist_id, request); // 実行時はpass不要
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, url, pre_confirm, pre_execute);
    }

    /*!
     *  @brief  注文訂正
     *  @param  order_id    注文番号(管理用)
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
    {
        // 注文入力 ※regist_id取得
        std::wstring url(BuildControlOrderURL(URL_BK_ORDER[ORDER_CORRECT][OSTEP_INPUT]));
        utility::AddItemToURL(PARAM_NAME_CORRECT_ORDER_ID, std::to_wstring(order_id), url);

        // 注文確認準備function
        const auto pre_confirm = [this, order_id](const StockOrder& order,
                                                  const std::wstring& pass,
                                                  int64_t regist_id,
                                                  const std::wstring& input_url,
                                                  web::http::http_request& request)->std::wstring
        {
            std::wstring cf_url(std::move(std::wstring(URL_BK_CORRECTORDER_CONFIRM)));
            utility::SetHttpCommonHeaderKeepAlive(cf_url, m_cookies_gr, input_url, request);
            BuildCorrectOrderFormData(order_id, order, pass, regist_id, request);
            return cf_url;
        };
        // 注文発行準備function
        const auto pre_execute = [this, order_id](const StockOrder& order,
                                                  int64_t regist_id,
                                                  const std::wstring& cf_url,
                                                  web::http::http_request& request)->std::wstring
        {
            std::wstring ex_url(std::move(std::wstring(URL_BK_CORRECTORDER_EXCUTE)));
            utility::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildCorrectOrderFormData(order_id, order, std::wstring(), regist_id, request); // 実行時はpass不要
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, url, pre_confirm, pre_execute);
    }

    /*!
     *  @brief  注文取消
     *  @param  order_id    注文番号(管理用)
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback)
    {
        // 取消入力 ※regist_id取得
        web::http::http_request request(web::http::methods::GET);
        std::wstring url(BuildControlOrderURL(URL_BK_ORDER[ORDER_CANCEL][OSTEP_INPUT]));
        utility::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);
        utility::AddItemToURL(PARAM_NAME_CANCEL_ORDER_ID, std::to_wstring(order_id), url);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, order_id, pwd, callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, url, order_id, pwd, callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);
                //
                const int64_t regist_id
                    = boost::python::extract<int64_t>(m_python.attr("getStockOrderRegistID")(inStringBuffer.collection(), static_cast<int32_t>(ORDER_CANCEL)));
                if (regist_id < 0) {
                    callback(false, RcvResponseStockOrder(), date_str);
                    return;
                }

                // 取消実行
                web::http::http_request request(web::http::methods::POST);
                std::wstring ex_url(URL_BK_CANCELORDER_EXCUTE);
                utility::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, url, request);
                BuildCancelOrderFormData(order_id, pwd, regist_id, request);
                web::http::client::http_client http_client(ex_url);
                http_client.request(request).then([this, ex_url, order_id, callback](web::http::http_response response)
                {
                    m_last_access_tick_mb = utility::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), ex_url);
                    utility::string_t date_str(response.headers().date());
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, order_id, callback](size_t bytesRead)
                    {
                        // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                        std::lock_guard<std::recursive_mutex> lock(m_mtx);
                        //
                        const boost::python::tuple t
                            = boost::python::extract<boost::python::tuple>(m_python.attr("responseStockOrderExec")(inStringBuffer.collection()));
                        RcvResponseStockOrder rcv_order;
                        const bool b_result = ToRcvResponseStockOrderFromResult_responseStockOrderExec(t, rcv_order);
                        rcv_order.m_order_id = order_id; // 取消responseはなぜか管理用注文番号を保持していない…
                        callback(b_result, rcv_order, date_str);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  SBIサイト最後アクセス時刻取得
     *  @return アクセス時刻(tickCount)
     *  @note   リクエスト応答受信時に更新
     *  @note   pcとmbでより古い方を返す
     */
    int64_t GetLastAccessTime() const
    {
        if (m_last_access_tick_pc > m_last_access_tick_mb) {
            return m_last_access_tick_mb;
        } else {
            return m_last_access_tick_pc;
        }
    }
};


/*!
 *  @param  script_mng  外部設定(スクリプト)管理者
 */
SecuritiesSessionSbi::SecuritiesSessionSbi(const TradeAssistantSetting& script_mng)
: m_pImpl(new PIMPL(script_mng))
{
}
/*!
 */
SecuritiesSessionSbi::~SecuritiesSessionSbi()
{
}

/*!
 *  @breif  ログイン
 *  @param  uid
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback)
{
    m_pImpl->Login(uid, pwd, callback);
}

/*!
 *  @brief  ポートフォリオ作成
 *  @param  monitoring_code     監視銘柄
 *  @param  investments_type    株取引所種別
 *  @param  callback            コールバック
 */
void SecuritiesSessionSbi::CreatePortfolio(const std::unordered_set<uint32_t>& monitoring_code,
                                           eStockInvestmentsType investments_type,
                                           const CreatePortfolioCallback& callback)
{
    m_pImpl->CreatePortfolio(monitoring_code, investments_type, callback);
}
/*!
 *  @brief  ポートフォリオ転送
 *  @param  callback    コールバック
 *  @note   mobileサイトからPCサイトへの転送
 *  @note   mobileサイトは表示項目が少なすぎるのでPCサイトへ移してそちらから情報を得たい
 */
void SecuritiesSessionSbi::TransmitPortfolio(const TransmitPortfolioCallback& callback)
{
    m_pImpl->TransmitPortfolio(callback);
}

/*!
 *  @brief  価格データ更新
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::UpdateValueData(const UpdateValueDataCallback& callback)
{
    m_pImpl->UpdateValueData(callback);
}

/*!
 *  @brief  新規売買注文
 *  @param  order       注文情報
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::FreshOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->FreshOrder(order, pwd, callback);
}

/*!
 *  @brief  注文訂正
 *  @param  order_id    注文番号(管理用)
 *  @param  order       注文情報
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->CorrectOrder(order_id, order, pwd, callback);
}

/*!
 *  @brief  注文取消
 *  @param  order_id    注文番号(管理用)
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->CancelOrder(order_id, pwd, callback);
}


/*!
 *  @brief  証券会社サイト最終アクセス時刻取得
 */
int64_t SecuritiesSessionSbi::GetLastAccessTime() const
{
    return m_pImpl->GetLastAccessTime();
}

} // namespace trading
