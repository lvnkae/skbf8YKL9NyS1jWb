/*!
 *  @file   holiday_investigator.cpp
 *  @brief  [common]今日が休日かどうか調査するクラス
 *  @date   2017/12/19
 */
#include "holiday_investigator.h"

#include "environment.h"

#include "utility/utility_debug.h"
#include "utility/utility_http.h"
#include "utility/utility_python.h"

#include "cpprest/http_client.h"
#include "cpprest/filestream.h"

#include <iostream>
#include <codecvt>

class HolidayInvestigator::PIMPL
{
private:
    boost::python::api::object m_python;    //!< pythonスクリプトオブジェクト
    HolidayInvestigator::CallBackFunction m_callback;   //!< 調査完了コールバック

    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

public:
    PIMPL()
    /* html解析用のpythonブジェクト生成 */
    : m_python(std::move(garnet::utility_python::PreparePythonScript(Environment::GetPythonConfig(), "holiday_investigator.py")))
    , m_callback()
    {
    }

    /*!
     *  @brief  調査要求を出す
     *  @param  function    調査完了コールバック
     */
    void Investigate(const CallBackFunction& function)
    {
        if (m_callback) {
            return; // 複数回は呼べない(error)
        }
        m_callback = function;
        //
        web::http::http_request request(web::http::methods::GET);
        garnet::utility_http::SetHttpCommonHeaderSimple(request);
        web::http::client::http_client http_client(L"https://yonelabo.com/today_holyday/");
        http_client.request(request).then([this](web::http::http_response response)
        {
            utility::string_t date_str(response.headers().date());
            concurrency::streams::istream bodyStream = response.body();
            concurrency::streams::container_buffer<std::string> inStringBuffer;
            return bodyStream.read_to_delim(inStringBuffer, 0).then([this, inStringBuffer, date_str](size_t bytesRead)
            {
                const boost::python::tuple t =
                    boost::python::extract<boost::python::tuple>(m_python.attr("investigateHoliday")(inStringBuffer.collection()));
                const int INX_RESULT = 0;   // investigateHoliday実行成否
                const int INX_HOLIDAY = 1;  // 休日(土日祝)フラグ
                m_callback(boost::python::extract<bool>(t[INX_RESULT]),
                           boost::python::extract<bool>(t[INX_HOLIDAY]),
                           date_str);
            });
        });
    }
};

/*!
 *  @brief
 */
HolidayInvestigator::HolidayInvestigator()
: m_pImpl(new PIMPL())
{
}

/*!
 *  @brief
 */
HolidayInvestigator::~HolidayInvestigator()
{
}

/*!
 *  @brief  調査要求を出す
 *  @param  function    調査完了コールバック
 */
void HolidayInvestigator::Investigate(const CallBackFunction& function)
{
    m_pImpl->Investigate(function);
}
