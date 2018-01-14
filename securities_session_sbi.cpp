/*!
 *  @file   securities_session_sbi.cpp
 *  @brief  SBI証券サイトとのセッション管理
 *  @date   2017/05/05
 */
#include "securities_session_sbi.h"

#include "environment.h"
#include "stock_portfolio.h"
#include "stock_holdings.h"
#include "trade_assistant_setting.h"
#include "trade_struct.h"
#include "trade_utility.h"

#include "http_cookies.h"
#include "garnet_time.h"
#include "yymmdd.h"
#include "utility/utility_datetime.h"
#include "utility/utility_debug.h"
#include "utility/utility_http.h"
#include "utility/utility_python.h"
#include "utility/utility_string.h"

#include "cpprest/http_client.h"
#include "cpprest/filestream.h"

#include <codecvt>
#include <mutex>

using namespace garnet;

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

    //! 外部設定から得る固定値
    const size_t m_max_code_register;           //!< 監視銘柄最大登録数
    const int32_t m_use_pf_number_monitoring;   //!< 銘柄監視に使用するポートフォリオ番号
    const int32_t m_pf_indicate_monitoring;     //!< ポートフォリオ表示形式：監視銘柄
    const int32_t m_pf_indicate_owned;          //!< ポートフォリオ表示形式：保有銘柄

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  汎用株注文
     *  @param  order           注文情報
     *  @param  pwd
     *  @param  callback        コールバック
     *  @param  input_url       注文入力URL
     *  @param  pre_confirm     注文確認前処理
     *  @param  pre_execute     注文発行前処理
     *  @note   現物売買/信用新規売買/信用返済売買/価格訂正に使用
     */
    typedef std::function<std::wstring (const StockOrder&, const std::wstring&, int64_t, const std::wstring&, web::http::http_request&)> PreOrderConfirm;
    typedef std::function<std::wstring (const StockOrder&, int64_t, const std::wstring&, web::http::http_request&)> PreOrderExecute;
    void StockOrderExecute(const StockOrder& order,
                           const std::wstring& pwd,
                           const OrderCallback& callback,
                           const std::wstring& input_url,
                           const PreOrderConfirm& pre_confirm,
                           const PreOrderExecute& pre_execute,
                           web::http::http_request& request)
    {
        if (!order.IsValid()) {
            callback(false, RcvResponseStockOrder(), std::wstring());    // 不正注文(error)
            return;
        }
        // 注文入力 ※regist_id取得
        web::http::client::http_client http_client(input_url);
        http_client.request(request).then([this, input_url, order, pwd, callback,
                                           pre_confirm, pre_execute](web::http::http_response response)
        {
            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
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
                    m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
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
                        http_client.request(request).then([this, ex_url, callback](web::http::http_response response)
                        {
                            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
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
                                const bool b_result = ToRcvResponseStockOrderFrom_responseStockOrderExec(t, rcv_order);
                                callback(b_result, rcv_order, date_str);
                            });
                        });
                    });
                });
            });
        });
    }

public:
    /*!
     *  @param  script_mng  外部設定(スクリプト)管理者
     */
    PIMPL(const TradeAssistantSetting& script_mng)
    /* html解析用のpythonブジェクト生成 */
    : m_python(std::move(utility_python::PreparePythonScript(Environment::GetPythonConfig(), "html_parser_sbi.py")))
    , m_cookies_gr()
    , m_last_access_tick_mb(0)
    , m_last_access_tick_pc(0)
    , m_max_code_register(script_mng.GetMaxMonitoringCodeRegister())
    , m_use_pf_number_monitoring(script_mng.GetUsePortfolioNumberForMonitoring())
    , m_pf_indicate_monitoring(script_mng.GetPortfolioIndicateForMonitoring())
    , m_pf_indicate_owned(script_mng.GetPortfolioIndicateForOwned())
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
        const std::wstring url(URL_BK_LOGIN);
        web::http::http_request request(web::http::methods::POST);
        utility_http::SetHttpCommonHeaderSimple(request);
        BuildLoginFormData(uid, pwd, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, uid, pwd, url, callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            const utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, uid, pwd, callback](size_t bytesRead)
            {
                using boost::python::tuple;
                using boost::python::extract;
                //
                const int INX_RESULT = 0;   // responseLoginMobile実行成否
                const int INX_LOGIN = 1;    // ログイン成否
                const std::string& html_u8(inStringBuffer.collection());
                const tuple t = extract<tuple>(m_python.attr("responseLoginMobile")(html_u8));
                bool b_result = extract<bool>(t[INX_RESULT]);
                bool b_login_result = extract<bool>(t[INX_LOGIN]);
                if (!b_result || !b_login_result) {
                    callback(b_result, b_login_result, false, date_str);
                    return;
                }

                // PCサイトログイン
                const std::wstring url(URL_MAIN_SBI_MAIN);
                web::http::http_request request(web::http::methods::POST);
                utility_http::SetHttpCommonHeaderSimple(request);
                BuildLoginPCFormData(uid, pwd, request);
                //
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    const utility::string_t date_str(response.headers().date());
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, callback](size_t bytesRead)
                    {
                        const int INX_RESULT = 0;       // responseLoginPC実行成否
                        const int INX_LOGIN = 1;        // ログイン成否
                        const int INX_IMPORT_MSG = 2;   // 重要なお知らせの有無
                        const std::string& html_sjis(inStringBuffer.collection());
                        const tuple t
                            = extract<tuple>(m_python.attr("responseLoginPC")(html_sjis));
                        callback(extract<bool>(t[INX_RESULT]),
                                 extract<bool>(t[INX_LOGIN]),
                                 extract<bool>(t[INX_IMPORT_MSG]),
                                 date_str);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  監視銘柄コード登録(mb)
     *  @param  monitoring_code     監視銘柄コード
     *  @param  investments_code    株取引所種別
     *  @param  callback            コールバック
     *  @note   mobileサイトで監視銘柄を登録する
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const RegisterMonitoringCodeCallback& callback)
    {
        // 登録確認(regist_id取得)
        const std::wstring url(std::move(std::wstring(URL_BK_BASE) + URL_BK_STOCKENTRYCONFIRM));
        web::http::http_request request(web::http::methods::POST);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);
        BuildDummyMonitoringCodeFormData(m_use_pf_number_monitoring, m_max_code_register, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this,
                                           monitoring_code,
                                           investments_type,
                                           url,
                                           callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
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
                    callback(false, StockBrandContainer());
                    return;
                }

                // 登録実行
                const std::wstring& url(std::move(std::wstring(URL_BK_BASE) + URL_BK_STOCKENTRYEXECUTE));
                web::http::http_request request(web::http::methods::POST);
                utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(URL_BK_BASE)+URL_BK_STOCKENTRYCONFIRM, request);
                BuildMonitoringCodeFormData(m_use_pf_number_monitoring, m_max_code_register, monitoring_code, investments_type, regist_id, request);
                //
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
                    {
                        StockBrandContainer rcv_brand_data;
                        const std::string& html_u8 = inStringBuffer.collection();
                        const boost::python::list rcv_data = boost::python::extract<boost::python::list>(m_python.attr("getPortfolioMobile")(html_u8));
                        const auto len = boost::python::len(rcv_data);
                        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
                        rcv_brand_data.reserve(len);
                        for (auto inx = 0; inx < len; inx++) {
                            const auto elem(std::move(rcv_data[inx]));
                            const std::string u8name(std::move(boost::python::extract<std::string>(elem[0])));
                            const std::wstring u16name(std::move(utfconv.from_bytes(u8name)));
                            const uint32_t code = boost::python::extract<uint32_t>(elem[1]);
                            rcv_brand_data.emplace(code, u16name);
                        }
                        callback(true, rcv_brand_data);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  監視銘柄コード転送(mb→PC)
     *  @param  callback    コールバック
     *  @note   mobileサイトからPCサイトへの転送
     *  @note   ※mobileサイトは登録シーケンスがシンプルだが表示項目が少ない
     *  @note   ※PCサイトは表示項目が揃ってるが登録シーケンスが複雑
     */
    void TransmitMonitoringCode(const std::function<void(bool)>& callback)
    {
        // PF転送入り口取得(site0へのログイン)
        // site0はログイン機能が非公開だが、「PF転送入り口」にアクセスすることでログインされる
        const std::wstring url(std::move(std::wstring(URL_MAIN_SBI_MAIN) + URL_MAIN_SBI_TRANS_PF_LOGIN));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), URL_MAIN_SBI_TRANS_PF_CHECK); // ここでsite0に移動…
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
            {
                const std::string& html_sjis(inStringBuffer.collection());
                const bool b_success
                    = m_python.attr("responseGetEntranceOfPortfolioTransmission")(html_sjis);
                if (!b_success) {
                    callback(false);
                    return;
                }
                // mb→PC転送
                const std::wstring url(URL_MAIN_SBI_TRANS_PF_EXEC);
                web::http::http_request request(web::http::methods::POST);
                utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_TRANS_PF_CHECK, request);
                BuildTransmitMonitoringCodeFormData(request);
                //
                web::http::client::http_client http_client(url);
                http_client.request(request).then([this, url, callback](web::http::http_response response)
                {
                    m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), url);
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
                    {
                        const std::string& html_sjis(inStringBuffer.collection());
                        const bool b_success
                            = m_python.attr("responseReqPortfolioTransmission")(html_sjis);
                        callback(b_success);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  保有株式情報取得
     *  @param  callback    コールバック
     */
    void GetStockOwned(const GetStockOwnedCallback& callback)
    {
        const std::wstring url(
            std::move(BuildPortfolioURL(PORTFOLIO_ID_OWNED, m_pf_indicate_owned)));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                using boost::python::tuple;
                using boost::python::extract;
                //
                const std::string& html_sjis = inStringBuffer.collection();
                const tuple t
                    = extract<tuple>(m_python.attr("getPortfolioPC_Owned")(html_sjis));
                const bool b_result = extract<bool>(t[0]);
                if (!b_result) {
                    callback(false, SpotTradingsStockContainer(), StockPositionContainer());
                    return;
                }
                SpotTradingsStockContainer spot;
                {
                    const boost::python::list l = extract<boost::python::list>(t[1]);
                    const auto len = boost::python::len(l);
                    spot.reserve(len);
                    for (auto inx = 0; inx < len; inx++) {
                        auto elem = l[inx];
                        // 現物保有情報<銘柄コード, 保有株数>
                        spot.emplace(extract<uint32_t>(elem[0]), extract<int32_t>(elem[1]));
                    }
                }
                StockPositionContainer position;
                {
                    const boost::python::list l = extract<boost::python::list>(t[2]);
                    const auto len = boost::python::len(l);
                    position.reserve(len);
                    for (auto inx = 0; inx < len; inx++) {
                        auto elem = l[inx];
                        StockPosition t_pos;
                        t_pos.m_code = std::move(StockCode(extract<uint32_t>(elem[0])));
                        t_pos.m_date
                            = std::move(garnet::YYMMDD::Create(extract<std::string>(elem[3])));
                        t_pos.m_value = extract<float64>(elem[4]);
                        t_pos.m_number = extract<int32_t>(elem[1]);
                        t_pos.m_b_sell = extract<bool>(elem[2]);
                        position.emplace_back(std::move(t_pos));
                    }
                }
                callback(true, spot, position);
            });
        });
    }

    /*!
     *  @brief  監視銘柄価格データ取得
     *  @param  callback    コールバック
     */
    void UpdateValueData(const UpdateValueDataCallback& callback)
    {
        const std::wstring url(
            std::move(BuildPortfolioURL(PORTFOLIO_ID_USER_TOP+m_use_pf_number_monitoring,
                                        m_pf_indicate_monitoring)));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            const utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                using boost::python::list;
                using boost::python::extract;
                //
                std::vector<RcvStockValueData> rcv_valuedata;
                const std::string& html_sjis = inStringBuffer.collection();
                const list l = extract<list>(m_python.attr("getPortfolioPC")(html_sjis));
                const auto len = boost::python::len(l);
                rcv_valuedata.reserve(len);
                for (auto inx = 0; inx < len; inx++) {
                    auto elem = l[inx];
                    RcvStockValueData rcv_vunit;
                    rcv_vunit.m_code = extract<uint32_t>(elem[0]);
                    rcv_vunit.m_value = extract<float64>(elem[1]);
                    rcv_vunit.m_open = extract<float64>(elem[2]);
                    rcv_vunit.m_high = extract<float64>(elem[3]);
                    rcv_vunit.m_low = extract<float64>(elem[4]);
                    rcv_vunit.m_close = extract<float64>(elem[5]);
                    rcv_vunit.m_volume = extract<int64_t>(elem[6]);
                    rcv_valuedata.push_back(rcv_vunit);
                }
                const bool b_sucess = !rcv_valuedata.empty();
                callback(b_sucess, date_str, rcv_valuedata);
            });
        });
    }

    /*!
     *  @brief  当日約定注文取得
     *  @param  callback    コールバック
     */
    void UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback)
    {
        const std::wstring url(
            std::move(std::wstring(URL_MAIN_SBI_MAIN) + URL_MAIN_SBI_ORDER_LIST));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, callback](web::http::http_response response)
        {
            garnet::sTime date_tm;
            const utility::string_t date_str(std::move(response.headers().date()));
            auto ptime = std::move(garnet::utility_datetime::ToLocalTimeFromRFC1123(date_str));
            garnet::utility_datetime::ToTimeFromBoostPosixTime(ptime, date_tm);
            //
            m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this,
                                                                     inStringBuffer, date_tm,
                                                                     callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                using boost::python::tuple;
                using boost::python::list;
                using boost::python::extract;
                //
                const std::string& html_sjis = inStringBuffer.collection();
                const tuple t = extract<tuple>(m_python.attr("getTodayExecInfo")(html_sjis));
                const bool b_result = extract<bool>(t[0]);
                if (!b_result) {
                    callback(false, std::vector<StockExecInfoAtOrder>());
                    return;
                }
                const list l = extract<list>(t[1]);
                const auto len = boost::python::len(l);
                garnet::sTime exe_tm;
                exe_tm.tm_year = date_tm.tm_year;
                std::vector<StockExecInfoAtOrder> rcv_data;
                rcv_data.reserve(len);
                for (auto inx = 0; inx < len; inx++) {
                    auto elem = l[inx];
                    StockExecInfoAtOrder exe_info;
                    exe_info.m_user_order_id = extract<int32_t>(elem[0]);
                    const int32_t i_odtype = extract<int32_t>(elem[1]);
                    exe_info.m_type = static_cast<eOrderType>(i_odtype);
                    const std::string investments_str = std::move(extract<std::string>(elem[2]));
                    exe_info.m_investments = GetStockInvestmentsTypeFromSbiCode(investments_str);
                    exe_info.m_code = extract<uint32_t>(elem[3]);
                    exe_info.m_b_leverage = extract<bool>(elem[4]);
                    exe_info.m_b_complete = extract<bool>(elem[5]);
                    const list exe_list = extract<list>(elem[6]);
                    const auto len_exe = boost::python::len(exe_list);
                    exe_info.m_exec.reserve(len_exe);
                    for (auto exe_inx = 0; exe_inx < len_exe; exe_inx++) {
                        auto exe_elem = exe_list[exe_inx];
                        const std::string datetime = std::move(extract<std::string>(exe_elem[0]));
                        const int32_t number = extract<int32_t>(exe_elem[1]);
                        const float64 value = extract<float64>(exe_elem[2]);
                        utility_datetime::ToTimeFromString(datetime, "%m/%d %H:%M:%S", exe_tm);
                        exe_info.m_exec.emplace_back(exe_tm, number, value);
                    }
                    rcv_data.emplace_back(std::move(exe_info));
                }
                callback(true, rcv_data);
            });
        });
    }

    /*!
     *  @brief  余力取得
     */
    void UpdateMargin(const UpdateMarginCallback& callback)
    {
        const std::wstring url(
            std::move(std::wstring(URL_BK_BASE) + URL_BK_POSITIONMARGIN));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_BK_BASE, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, callback](web::http::http_response response)
        {
            const int64_t rcv_tick = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this,
                                                                     inStringBuffer, rcv_tick,
                                                                     callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                const std::string& html_u8 = inStringBuffer.collection();
                const bool b_result =
                    boost::python::extract<bool>(m_python.attr("getMarginMobile")(html_u8));
                if (b_result) {
                    // 成功したら最終アクセス時刻更新
                    m_last_access_tick_mb = rcv_tick;
                }
                callback(b_result);
            });
        });
    }

    /*!
     *  @brief  株売買注文
     *  @param  order       注文情報
     *  @param  pwd
     *  @param  callback    コールバック
     */
    void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
    {
        // 注文入力 ※regist_id取得
        std::wstring url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_INPUT)));
        utility_http::AddItemToURL(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), url);
        utility_http::AddItemToURL(PARAM_NAME_ORDER_INVESTIMENTS, GetSbiInvestimentsCode(order.m_investments), url);
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);

        // 注文確認準備function
        const auto pre_confirm = [this](const StockOrder& order,
                                        const std::wstring& pass,
                                        int64_t regist_id,
                                        const std::wstring& input_url,
                                        web::http::http_request& request)->std::wstring
        {
            std::wstring cf_url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_CONFIRM)));
            utility_http::SetHttpCommonHeaderKeepAlive(cf_url, m_cookies_gr, input_url, request);
            BuildFreshOrderFormData(order, pass, regist_id, request);
            return cf_url;
        };
        // 注文発行準備function
        const auto pre_execute = [this](const StockOrder& order,
                                        int64_t regist_id,
                                        const std::wstring& cf_url,
                                        web::http::http_request& request)->std::wstring
        {
            std::wstring ex_url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_EXECUTE)));
            utility_http::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildFreshOrderFormData(order, std::wstring(), regist_id, request); // 実行時はpass不要
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, url, pre_confirm, pre_execute, request);
    }

    /*!
     *  @brief  株信用返済建玉取得
     *  @param  yymmdd  建日
     *  @param  value   建単価
     *  @param  order   注文情報
     *  @param  callback
     */
    typedef std::function<void (const std::wstring&, bool, uint32_t, const std::string&, const std::string&)> SelTatedamaCallback;
    void SelectTatedama(const garnet::YYMMDD& yymmdd,
                        float64 value,
                        const StockOrder& order,
                        const SelTatedamaCallback& callback)
    {
        std::wstring url(std::move(BuildRepOrderTateListURL(order.m_type)));
        utility_http::AddItemToURL(PARAM_NAME_LEVERAGE_CATEGORY, L"6", url);   // >ToDo< 一般信用/日計りの対応
        utility_http::AddItemToURL(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), url);
        url += L"&open_trade_kbn=1&open_market=TKY&caIsLump=false&request_type=16";
        //
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, yymmdd, value, callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), url);
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, url, yymmdd, value, callback](size_t bytesRead)
            {
                // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                using boost::python::tuple;
                using boost::python::extract;
                //
                const std::string html_u8(inStringBuffer.collection());
                const char func_name[] = "responseRepLeverageStockOrderTateList";
                const tuple t = extract<tuple>(m_python.attr(func_name)(html_u8));
                //
                const bool b_result = extract<bool>(t[0]);
                if (b_result) {
                    const uint32_t rcv_code = extract<uint32_t>(t[1]);
                    const std::string caIQ(std::move(extract<std::string>(t[2])));
                    const boost::python::list tatedama = extract<boost::python::list>(t[3]);
                    const auto len = boost::python::len(tatedama);
                    for (auto inx = 0; inx < len; inx++) {
                        const auto elem(std::move(tatedama[inx]));
                        const std::string t_yymmddstr(std::move(extract<std::string>(elem[0])));
                        const float64 t_value = extract<float64>(elem[1]);
                        //const int32_t t_number = extract<int32_t>(elem[2]);
                        const garnet::YYMMDD t_yymmdd(std::move(garnet::YYMMDD::Create(t_yymmddstr)));
                        // 指定された建日・建単価の玉を返済する
                        if (trade_utility::same_value(value, t_value) && yymmdd == t_yymmdd) {
                            callback(url,
                                     true,
                                     rcv_code, caIQ, std::move(extract<std::string>(elem[3])));
                            return;
                        }
                    }
                }
                callback(std::wstring(),
                         false,
                         StockCode().GetCode(), std::string(), std::string());
            });
        });
    }

    /*!
     *  @brief  株信用返済注文
     *  @param  caIQ            form data用キー
     *  @param  quantity_tag    同上
     *  @param  referer         リファラ
     *  @param  order           注文情報
     *  @param  pwd
     *  @param  callback        コールバック
     */
    void RepaymentLeverageOrder(const std::string& caIQ,
                                const std::string& quantity_tag,
                                const std::wstring& referer,
                                const StockOrder& order,
                                const std::wstring& pwd,
                                const OrderCallback& callback)
    {
        // 注文入力 ※regist_id取得
        web::http::http_request request(web::http::methods::POST);
        std::wstring input_url(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_INPUT));
        utility_http::SetHttpCommonHeaderKeepAlive(input_url, m_cookies_gr, referer, request);
        BuildRepaymenLeverageOrderFormdata(caIQ, quantity_tag, order, std::wstring(), -1, request);

        // 注文確認準備function
        const auto pre_confirm = [this, caIQ](const StockOrder& order,
                                              const std::wstring& pass,
                                              int64_t regist_id,
                                              const std::wstring& input_url,
                                              web::http::http_request& request)->std::wstring
        {
            std::wstring cf_url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_CONFIRM)));
            utility_http::SetHttpCommonHeaderKeepAlive(cf_url, m_cookies_gr, input_url, request);
            BuildRepaymenLeverageOrderFormdata(caIQ, std::string(), order, pass, regist_id, request);
            return cf_url;
        };
        // 注文発行準備function
        const auto pre_execute = [this, caIQ](const StockOrder& order,
                                              int64_t regist_id,
                                              const std::wstring& cf_url,
                                              web::http::http_request& request)->std::wstring
        {
            std::wstring ex_url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_EXECUTE)));
            utility_http::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildRepaymenLeverageOrderFormdata(caIQ, std::string(), order, std::wstring(), regist_id, request);
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, input_url, pre_confirm, pre_execute, request);
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
        utility_http::AddItemToURL(PARAM_NAME_CORRECT_ORDER_ID, std::to_wstring(order_id), url);
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);

        // 注文確認準備function
        const auto pre_confirm = [this, order_id](const StockOrder& order,
                                                  const std::wstring& pass,
                                                  int64_t regist_id,
                                                  const std::wstring& input_url,
                                                  web::http::http_request& request)->std::wstring
        {
            std::wstring cf_url(std::move(std::wstring(URL_BK_CORRECTORDER_CONFIRM)));
            utility_http::SetHttpCommonHeaderKeepAlive(cf_url, m_cookies_gr, input_url, request);
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
            utility_http::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildCorrectOrderFormData(order_id, order, std::wstring(), regist_id, request);
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, url, pre_confirm, pre_execute, request);
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
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);
        utility_http::AddItemToURL(PARAM_NAME_CANCEL_ORDER_ID, std::to_wstring(order_id), url);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, url, order_id, pwd, callback](web::http::http_response response)
        {
            m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
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
                utility_http::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, url, request);
                BuildCancelOrderFormData(order_id, pwd, regist_id, request);
                web::http::client::http_client http_client(ex_url);
                http_client.request(request).then([this, ex_url, order_id, callback](web::http::http_response response)
                {
                    m_last_access_tick_mb = utility_datetime::GetTickCountGeneral();
                    m_cookies_gr.Set(response.headers(), ex_url);
                    utility::string_t date_str(response.headers().date());
                    concurrency::streams::istream bodyStream = response.body();
                    concurrency::streams::container_buffer<std::string> inStringBuffer;
                    return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str, order_id, callback](size_t bytesRead)
                    {
                        // python関数多重呼び出しが起こり得る(ステップ実行中は特に)のでロックしておく
                        std::lock_guard<std::recursive_mutex> lock(m_mtx);
                        //
                        const std::string& html_u8(inStringBuffer.collection());
                        using boost::python::tuple;
                        using boost::python::extract;
                        const tuple t
                            = extract<tuple>(m_python.attr("responseStockOrderExec")(html_u8));
                        RcvResponseStockOrder rcv_order;
                        const bool b_result
                            = ToRcvResponseStockOrderFrom_responseStockOrderExec(t, rcv_order);
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
 *  @brief  監視銘柄コード登録
 *  @param  monitoring_code     監視銘柄コード
 *  @param  investments_type    株取引所種別
 *  @param  callback            コールバック
 */
void SecuritiesSessionSbi::RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                                  eStockInvestmentsType investments_type,
                                                  const RegisterMonitoringCodeCallback& callback)
{
    // pplx::taskを7段以上繋ぐとstack不足の例外が出るので、登録と転送で処理を分ける
    m_pImpl->RegisterMonitoringCode(monitoring_code, investments_type,
                                    [this, callback](bool b_result, const StockBrandContainer& rcv_data)
    {
        if (!b_result) {
            callback(b_result, StockBrandContainer());
        } else {
            m_pImpl->TransmitMonitoringCode([callback, rcv_data](bool b_result)
            {  
                if (!b_result) {
                    callback(false, StockBrandContainer());
                } else {
                    callback(true, rcv_data);
                } 
            });
        }
    });
}

/*!
 *  @brief  保有株式情報取得
 */
void SecuritiesSessionSbi::GetStockOwned(const GetStockOwnedCallback& callback)
{
    m_pImpl->GetStockOwned(callback);
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
 *  @brief  約定情報取得取得
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback)
{
    m_pImpl->UpdateExecuteInfo(callback);
}
/*!
 *  @brief  余力取得
 */
void SecuritiesSessionSbi::UpdateMargin(const UpdateMarginCallback& callback)
{
    m_pImpl->UpdateMargin(callback);
}

/*!
 *  @brief  売買注文
 *  @param  order       注文情報
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->BuySellOrder(order, pwd, callback);
}

/*!
 *  @brief  信用返済注文
 *  @param  t_yymmdd    建日
 *  @param  t_value     建単価
 *  @param  order       注文情報
 *  @param  pwd
 *  @param  callback    コールバック
 */
void SecuritiesSessionSbi::RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                                  const StockOrder& order,
                                                  const std::wstring& pwd,
                                                  const OrderCallback& callback)
{
    if (!order.IsValid()) {
        // 不正注文(error)
        callback(false, RcvResponseStockOrder(), std::wstring());
        return;
    }
    // pplx::taskを7段以上繋ぐとstack不足の例外が出るので、建玉選択と返済発注で処理を分ける
    m_pImpl->SelectTatedama(t_yymmdd, t_value, order,
                            [this, order, pwd, callback](const std::wstring& tate_url,
                                                         bool b_result,
                                                         uint32_t rcv_code, const std::string& caIQ, const std::string& qt_tag)
    {
        if (b_result && rcv_code == order.m_code.GetCode()) {
            m_pImpl->RepaymentLeverageOrder(caIQ, qt_tag, tate_url, order, pwd, callback);
        } else {
            // 失敗(error)
            callback(false, RcvResponseStockOrder(), std::wstring());
        }
    });
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
