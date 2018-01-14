/*!
*  @file   python_config.cpp
*  @brief  [common]python設定
*  @date   2017/12/11
*/
#include "python_config.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"
#include "boost/optional.hpp"

#include <fstream>
#if defined(PYTHON_USE_WCHAR)
#include <codecvt>
#endif/* PYTHON_USE_WCHAR */

namespace garnet
{

/*!
 *  @brief  空生成
 */
python_config::python_config()
: m_install_path()
, m_script_path()
{
}

/*!
 *  @param  iniファイル名
 */
python_config::python_config(const std::string& ini_file_name)
: m_install_path()
, m_script_path()
{
    initialize(ini_file_name);
}

/*!
 *  @brief  初期化
 *  @param  iniファイル名
 */
void python_config::initialize(const std::string& ini_file_name)
{
    std::ifstream ifs(ini_file_name);
    if (!ifs.is_open()) {
        return; // iniファイルがない
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(ini_file_name, pt);

    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Path.PythonHome");
#if defined(PYTHON_USE_WCHAR)
        std::string python_home(str.get());
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
        m_install_path = utfconv.from_bytes(python_home);
#else
        m_install_path = str.get();
#endif/* PYTHON_USE_WCHAR */
    }
    {
        boost::optional<std::string> str = pt.get_optional<std::string>("Path.PythonScript");
        m_script_path = str.get();
    }
}

} // namespace garnet
