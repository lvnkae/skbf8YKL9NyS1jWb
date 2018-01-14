/*!
 *  @file   python_config.h
 *  @brief  [common]python設定
 *  @date   2018/01/14
 *  @note   boost に依存してる
 */
#pragma once

#include "python_define.h"
#include <string>

namespace garnet
{
/*!
 *  @brief  python設定
 *  @note   iniファイルを読み込んで外部に提供
 */
class python_config
{
public:
    /*!
     *  @brief  空生成
     */
    python_config();
    /*!
     *  @param  iniファイル名
     */
    python_config(const std::string& ini_file_name);

    /*!
     *  @brief  pythonのインストールパス(full)を得る
     *  @return インストールパスポインタ
     *  @note   戻り値が非const生ポなのはPy_SetPythonHomeがそれを要求するため…
     *  @note   型はpython2.X系以前とpython3.X以降で異なる
     */
    PYCHAR* GetPythonHome() const
    {
        return const_cast<PYCHAR*>(m_install_path.c_str());
    }
    /*!
     *  @brief  pythonスクリプトパスを得る
     */
    const std::string& GetPythonScriptPath() const
    {
        return m_script_path;
    }

private:
    python_config(const python_config&);
    python_config(python_config&&);
    python_config& operator= (const python_config&);

    /*!
     *  @brief  初期化
     *  @param  iniファイル名
     */
    void initialize(const std::string& ini_file_name);

    PYSTRING m_install_path;    //!< pythonインストールパス
    std::string m_script_path;  //!< pythonスクリプトパス
};

} // namespace garnet
