/*!
 *  @file   utility_python.h
 *  @brief  [common]python絡みのUtility関数
 *  @date   2017/05/13
 *  @note   boost::python に依存している
 */
#pragma once

#include <string>
#include "boost/python.hpp"

/*!
 *  @brief  pythonのスクリプトを使えるようにする
 *  @param  python_home     pythonのインストールパス(full)
 *  @param  python_script   pythonのスクリプトファイル名
 *  @return pythonアクセスオブジェクト
 *  @note   python_homeが非const生ポなのはPy_SetPythonHomeの都合…
 *  @note   しかもstaticなworkでないとNG、auto変数を渡さないように
 */
boost::python::api::object PreparePythonScript(PYCHAR* python_home, const std::string& python_script);

/*!
 *  @brief  pythonスクリプトエラー出力
 *  @param  e   エラー
 *  @note   boost::pythonはpythonが投げてきた例外(主にスクリプト内で発生したエラー)を出力する
 */
void OutputPythonError(boost::python::error_already_set& e);
