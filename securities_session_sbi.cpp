/*!
 *  @file   securities_session_sbi.cpp
 *  @brief  SBI�،��T�C�g�Ƃ̃Z�b�V�����Ǘ�
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
    std::recursive_mutex m_mtx;             //!< �r������q
    boost::python::api::object m_python;    //!< python�X�N���v�g�I�u�W�F�N�g
    web::http::cookies_group m_cookies_gr;  //!< SBI�pcookie�Q
    int64_t m_last_access_tick_mb;          //!< SBI(mobile)�֍Ō�ɃA�N�Z�X��������(tickCount)
    int64_t m_last_access_tick_pc;          //!< SBI(PC)�֍Ō�ɃA�N�Z�X��������(tickCount)

    //! �O���ݒ肩�瓾��Œ�l
    const size_t m_max_code_register;           //!< �Ď������ő�o�^��
    const int32_t m_use_pf_number_monitoring;   //!< �����Ď��Ɏg�p����|�[�g�t�H���I�ԍ�
    const int32_t m_pf_indicate_monitoring;     //!< �|�[�g�t�H���I�\���`���F�Ď�����
    const int32_t m_pf_indicate_owned;          //!< �|�[�g�t�H���I�\���`���F�ۗL����

private:
    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

    /*!
     *  @brief  �ėp������
     *  @param  order           �������
     *  @param  pwd
     *  @param  callback        �R�[���o�b�N
     *  @param  input_url       ��������URL
     *  @param  pre_confirm     �����m�F�O����
     *  @param  pre_execute     �������s�O����
     *  @note   ��������/�M�p�V�K����/�M�p�ԍϔ���/���i�����Ɏg�p
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
            callback(false, RcvResponseStockOrder(), std::wstring());    // �s������(error)
            return;
        }
        // �������� ��regist_id�擾
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                const int32_t i_order_type = static_cast<int32_t>(order.m_type);
                const int64_t regist_id
                    = boost::python::extract<int64_t>(m_python.attr("getStockOrderRegistID")(inStringBuffer.collection(), i_order_type));
                if (regist_id < 0) {
                    callback(false, RcvResponseStockOrder(), date_str);
                    return;
                }

                // �����m�F ��regist_id�擾
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
                        // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
                        std::lock_guard<std::recursive_mutex> lock(m_mtx);

                        const int32_t i_order_type = static_cast<int32_t>(order.m_type);
                        const int64_t regist_id
                            = boost::python::extract<int64_t>(m_python.attr("getStockOrderConfirmRegistID")(inStringBuffer.collection(), i_order_type));
                        if (regist_id < 0) {
                            callback(false, RcvResponseStockOrder(), date_str);
                            return;
                        }

                        // �������s
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
                                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
     *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
     */
    PIMPL(const TradeAssistantSetting& script_mng)
    /* html��͗p��python�u�W�F�N�g���� */
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
     *  @breif  ���O�C��
     *  @param  uid
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback)
    {
        // mobile�T�C�g���O�C��
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
                const int INX_RESULT = 0;   // responseLoginMobile���s����
                const int INX_LOGIN = 1;    // ���O�C������
                const std::string& html_u8(inStringBuffer.collection());
                const tuple t = extract<tuple>(m_python.attr("responseLoginMobile")(html_u8));
                bool b_result = extract<bool>(t[INX_RESULT]);
                bool b_login_result = extract<bool>(t[INX_LOGIN]);
                if (!b_result || !b_login_result) {
                    callback(b_result, b_login_result, false, date_str);
                    return;
                }

                // PC�T�C�g���O�C��
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
                        const int INX_RESULT = 0;       // responseLoginPC���s����
                        const int INX_LOGIN = 1;        // ���O�C������
                        const int INX_IMPORT_MSG = 2;   // �d�v�Ȃ��m�点�̗L��
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
     *  @brief  �Ď������R�[�h�o�^(mb)
     *  @param  monitoring_code     �Ď������R�[�h
     *  @param  investments_code    ����������
     *  @param  callback            �R�[���o�b�N
     *  @note   mobile�T�C�g�ŊĎ�������o�^����
     */
    void RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                eStockInvestmentsType investments_type,
                                const RegisterMonitoringCodeCallback& callback)
    {
        // �o�^�m�F(regist_id�擾)
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

                // �o�^���s
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
     *  @brief  �Ď������R�[�h�]��(mb��PC)
     *  @param  callback    �R�[���o�b�N
     *  @note   mobile�T�C�g����PC�T�C�g�ւ̓]��
     *  @note   ��mobile�T�C�g�͓o�^�V�[�P���X���V���v�������\�����ڂ����Ȃ�
     *  @note   ��PC�T�C�g�͕\�����ڂ������Ă邪�o�^�V�[�P���X�����G
     */
    void TransmitMonitoringCode(const std::function<void(bool)>& callback)
    {
        // PF�]��������擾(site0�ւ̃��O�C��)
        // site0�̓��O�C���@�\������J�����A�uPF�]��������v�ɃA�N�Z�X���邱�ƂŃ��O�C�������
        const std::wstring url(std::move(std::wstring(URL_MAIN_SBI_MAIN) + URL_MAIN_SBI_TRANS_PF_LOGIN));
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, URL_MAIN_SBI_MAIN, request);
        //
        web::http::client::http_client http_client(url);
        http_client.request(request).then([this, callback](web::http::http_response response)
        {
            m_last_access_tick_pc = utility_datetime::GetTickCountGeneral();
            m_cookies_gr.Set(response.headers(), URL_MAIN_SBI_TRANS_PF_CHECK); // ������site0�Ɉړ��c
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
                // mb��PC�]��
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
     *  @brief  �ۗL�������擾
     *  @param  callback    �R�[���o�b�N
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
                        // �����ۗL���<�����R�[�h, �ۗL����>
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
     *  @brief  �Ď��������i�f�[�^�擾
     *  @param  callback    �R�[���o�b�N
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
     *  @brief  ������蒍���擾
     *  @param  callback    �R�[���o�b�N
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
     *  @brief  �]�͎擾
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
                std::lock_guard<std::recursive_mutex> lock(m_mtx);

                const std::string& html_u8 = inStringBuffer.collection();
                const bool b_result =
                    boost::python::extract<bool>(m_python.attr("getMarginMobile")(html_u8));
                if (b_result) {
                    // ����������ŏI�A�N�Z�X�����X�V
                    m_last_access_tick_mb = rcv_tick;
                }
                callback(b_result);
            });
        });
    }

    /*!
     *  @brief  ����������
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
    {
        // �������� ��regist_id�擾
        std::wstring url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_INPUT)));
        utility_http::AddItemToURL(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), url);
        utility_http::AddItemToURL(PARAM_NAME_ORDER_INVESTIMENTS, GetSbiInvestimentsCode(order.m_investments), url);
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);

        // �����m�F����function
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
        // �������s����function
        const auto pre_execute = [this](const StockOrder& order,
                                        int64_t regist_id,
                                        const std::wstring& cf_url,
                                        web::http::http_request& request)->std::wstring
        {
            std::wstring ex_url(std::move(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_EXECUTE)));
            utility_http::SetHttpCommonHeaderKeepAlive(ex_url, m_cookies_gr, cf_url, request);
            BuildFreshOrderFormData(order, std::wstring(), regist_id, request); // ���s����pass�s�v
            return ex_url;
        };

        StockOrderExecute(order, pwd, callback, url, pre_confirm, pre_execute, request);
    }

    /*!
     *  @brief  ���M�p�ԍό��ʎ擾
     *  @param  yymmdd  ����
     *  @param  value   ���P��
     *  @param  order   �������
     *  @param  callback
     */
    typedef std::function<void (const std::wstring&, bool, uint32_t, const std::string&, const std::string&)> SelTatedamaCallback;
    void SelectTatedama(const garnet::YYMMDD& yymmdd,
                        float64 value,
                        const StockOrder& order,
                        const SelTatedamaCallback& callback)
    {
        std::wstring url(std::move(BuildRepOrderTateListURL(order.m_type)));
        utility_http::AddItemToURL(PARAM_NAME_LEVERAGE_CATEGORY, L"6", url);   // >ToDo< ��ʐM�p/���v��̑Ή�
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
                        // �w�肳�ꂽ�����E���P���̋ʂ�ԍς���
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
     *  @brief  ���M�p�ԍϒ���
     *  @param  caIQ            form data�p�L�[
     *  @param  quantity_tag    ����
     *  @param  referer         ���t�@��
     *  @param  order           �������
     *  @param  pwd
     *  @param  callback        �R�[���o�b�N
     */
    void RepaymentLeverageOrder(const std::string& caIQ,
                                const std::string& quantity_tag,
                                const std::wstring& referer,
                                const StockOrder& order,
                                const std::wstring& pwd,
                                const OrderCallback& callback)
    {
        // �������� ��regist_id�擾
        web::http::http_request request(web::http::methods::POST);
        std::wstring input_url(BuildOrderURL(order.m_b_leverage, order.m_type, OSTEP_INPUT));
        utility_http::SetHttpCommonHeaderKeepAlive(input_url, m_cookies_gr, referer, request);
        BuildRepaymenLeverageOrderFormdata(caIQ, quantity_tag, order, std::wstring(), -1, request);

        // �����m�F����function
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
        // �������s����function
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
     *  @brief  ��������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  order       �������
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
    {
        // �������� ��regist_id�擾
        std::wstring url(BuildControlOrderURL(URL_BK_ORDER[ORDER_CORRECT][OSTEP_INPUT]));
        utility_http::AddItemToURL(PARAM_NAME_CORRECT_ORDER_ID, std::to_wstring(order_id), url);
        web::http::http_request request(web::http::methods::GET);
        utility_http::SetHttpCommonHeaderKeepAlive(url, m_cookies_gr, std::wstring(), request);

        // �����m�F����function
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
        // �������s����function
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
     *  @brief  �������
     *  @param  order_id    �����ԍ�(�Ǘ��p)
     *  @param  pwd
     *  @param  callback    �R�[���o�b�N
     */
    void CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback)
    {
        // ������� ��regist_id�擾
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
                // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
                std::lock_guard<std::recursive_mutex> lock(m_mtx);
                //
                const int64_t regist_id
                    = boost::python::extract<int64_t>(m_python.attr("getStockOrderRegistID")(inStringBuffer.collection(), static_cast<int32_t>(ORDER_CANCEL)));
                if (regist_id < 0) {
                    callback(false, RcvResponseStockOrder(), date_str);
                    return;
                }

                // ������s
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
                        // python�֐����d�Ăяo�����N���蓾��(�X�e�b�v���s���͓���)�̂Ń��b�N���Ă���
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
                        rcv_order.m_order_id = order_id; // ���response�͂Ȃ����Ǘ��p�����ԍ���ێ����Ă��Ȃ��c
                        callback(b_result, rcv_order, date_str);
                    });
                });
            });
        });
    }

    /*!
     *  @brief  SBI�T�C�g�Ō�A�N�Z�X�����擾
     *  @return �A�N�Z�X����(tickCount)
     *  @note   ���N�G�X�g������M���ɍX�V
     *  @note   pc��mb�ł��Â�����Ԃ�
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
 *  @param  script_mng  �O���ݒ�(�X�N���v�g)�Ǘ���
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
 *  @breif  ���O�C��
 *  @param  uid
 *  @param  pwd
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::Login(const std::wstring& uid, const std::wstring& pwd, const LoginCallback& callback)
{
    m_pImpl->Login(uid, pwd, callback);
}

/*!
 *  @brief  �Ď������R�[�h�o�^
 *  @param  monitoring_code     �Ď������R�[�h
 *  @param  investments_type    ����������
 *  @param  callback            �R�[���o�b�N
 */
void SecuritiesSessionSbi::RegisterMonitoringCode(const StockCodeContainer& monitoring_code,
                                                  eStockInvestmentsType investments_type,
                                                  const RegisterMonitoringCodeCallback& callback)
{
    // pplx::task��7�i�ȏ�q����stack�s���̗�O���o��̂ŁA�o�^�Ɠ]���ŏ����𕪂���
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
 *  @brief  �ۗL�������擾
 */
void SecuritiesSessionSbi::GetStockOwned(const GetStockOwnedCallback& callback)
{
    m_pImpl->GetStockOwned(callback);
}

/*!
 *  @brief  ���i�f�[�^�X�V
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::UpdateValueData(const UpdateValueDataCallback& callback)
{
    m_pImpl->UpdateValueData(callback);
}

/*!
 *  @brief  �����擾�擾
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::UpdateExecuteInfo(const UpdateStockExecInfoCallback& callback)
{
    m_pImpl->UpdateExecuteInfo(callback);
}
/*!
 *  @brief  �]�͎擾
 */
void SecuritiesSessionSbi::UpdateMargin(const UpdateMarginCallback& callback)
{
    m_pImpl->UpdateMargin(callback);
}

/*!
 *  @brief  ��������
 *  @param  order       �������
 *  @param  pwd
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::BuySellOrder(const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->BuySellOrder(order, pwd, callback);
}

/*!
 *  @brief  �M�p�ԍϒ���
 *  @param  t_yymmdd    ����
 *  @param  t_value     ���P��
 *  @param  order       �������
 *  @param  pwd
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::RepaymentLeverageOrder(const garnet::YYMMDD& t_yymmdd, float64 t_value,
                                                  const StockOrder& order,
                                                  const std::wstring& pwd,
                                                  const OrderCallback& callback)
{
    if (!order.IsValid()) {
        // �s������(error)
        callback(false, RcvResponseStockOrder(), std::wstring());
        return;
    }
    // pplx::task��7�i�ȏ�q����stack�s���̗�O���o��̂ŁA���ʑI���ƕԍϔ����ŏ����𕪂���
    m_pImpl->SelectTatedama(t_yymmdd, t_value, order,
                            [this, order, pwd, callback](const std::wstring& tate_url,
                                                         bool b_result,
                                                         uint32_t rcv_code, const std::string& caIQ, const std::string& qt_tag)
    {
        if (b_result && rcv_code == order.m_code.GetCode()) {
            m_pImpl->RepaymentLeverageOrder(caIQ, qt_tag, tate_url, order, pwd, callback);
        } else {
            // ���s(error)
            callback(false, RcvResponseStockOrder(), std::wstring());
        }
    });
}

/*!
 *  @brief  ��������
 *  @param  order_id    �����ԍ�(�Ǘ��p)
 *  @param  order       �������
 *  @param  pwd
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::CorrectOrder(int32_t order_id, const StockOrder& order, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->CorrectOrder(order_id, order, pwd, callback);
}

/*!
 *  @brief  �������
 *  @param  order_id    �����ԍ�(�Ǘ��p)
 *  @param  pwd
 *  @param  callback    �R�[���o�b�N
 */
void SecuritiesSessionSbi::CancelOrder(int32_t order_id, const std::wstring& pwd, const OrderCallback& callback)
{
    m_pImpl->CancelOrder(order_id, pwd, callback);
}


/*!
 *  @brief  �،���ЃT�C�g�ŏI�A�N�Z�X�����擾
 */
int64_t SecuritiesSessionSbi::GetLastAccessTime() const
{
    return m_pImpl->GetLastAccessTime();
}

} // namespace trading
