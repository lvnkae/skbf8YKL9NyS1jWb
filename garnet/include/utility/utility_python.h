/*!
 *  @file   utility_python.h
 *  @brief  [common]python絡みのUtility関数
 *  @date   2017/05/13
 *  @note   boost::python に依存している
 */
#pragma once

#include "python/python_define.h"
#include "python/python_config_fwd.h"

#include <string>
#include "boost/python.hpp"

namespace garnet
{
class python_config;

namespace utility_python
{
/*!
 *  @brief  pythonのスクリプトを使えるようにする
 *  @param  python_home     python設定
 *  @param  python_script   pythonのスクリプトファイル名
 *  @return pythonアクセスオブジェクト
 */
boost::python::api::object PreparePythonScript(const python_config_ref& config, const std::string& python_script);

/*!
 *  @brief  pythonのスクリプトを使えるようにする(パス直指定版/非推奨)
 *  @param  python_home     pythonのインストールパス(full)
 *  @param  python_script   pythonのスクリプトファイル名
 *  @return pythonアクセスオブジェクト
 *  @note   python_confiを引数に取る関数の使用を推奨
 *  @note   ※python_homeが非const生ポなのはPy_SetPythonHomeの都合…
 *  @note   ※しかもstaticなworkでないとNG、auto変数を渡さないように
 */
boost::python::api::object PreparePythonScript(PYCHAR* python_home, const std::string& python_script);

/*!
 *  @brief  pythonスクリプトエラー出力
 *  @param  e   エラー
 *  @note   boost::pythonが投げてきた例外(主にスクリプト内で発生したエラー)を出力する
 */
void OutputPythonError(boost::python::error_already_set& e);

} // namespace utility_python
} // namespace garnet
