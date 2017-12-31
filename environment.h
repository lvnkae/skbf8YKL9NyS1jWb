/*!
 *  @file   environment.h
 *  @brief  環境設定
 *  @date   2017/12/11
 *  @note   iniファイル仲介
 */
#pragma once

#include <memory>
#include <string>

class Environment;

/*!
 *  @note   singleton
 *  @note   インスタンスは明示的に生成する(呼出側がshared_ptrで保持)
 *  @note   "初回インスタンス取得時生成"ではない
 */
class Environment
{
public:
    /*!
     *  @brief  インスタンス生成 + 初期化
     *  @return インスタンス共有ポインタ
     *  @note   戻したshared_ptrが有効な間は空shared_ptrを返す
     */
    static std::shared_ptr<Environment> Create();
    /*!
     *  @brief  インスタンス取得
     *  @return インスタンス弱参照(const)
     *  @note   生成前に呼ばれたら空weak_ptrを返す
     */
    static std::weak_ptr<const Environment> GetInstance() { return m_pInstance; }

    /*!
     *  @brief  トレーディングスクリプト名を得る
     */
    std::string GetTradingScript() const { return m_trading_script; }

    /*!
     *  @brief  Pythonのインストールパス(full)を得る
     */
    static PYCHAR* GetPythonHome() {
        std::shared_ptr<const Environment> p = Environment::GetInstance().lock();
        if (nullptr == p) {
            return nullptr;
        }
        //  戻り値が非constなのはPy_SetPythonHomeがそれを要求するため…
        return const_cast<PYCHAR*>(p->m_python_home.c_str());
    }

private:
    Environment();
    Environment(const Environment&);
    Environment& operator= (const Environment&);

    /*!
     *  @brief  初期化
     */
    void Initialize();

    PYSTRING m_python_home;         //! Pythonインストールパス
    std::string m_trading_script;   //! トレーディングスクリプト名

    static std::weak_ptr<Environment> m_pInstance; //! 自身の弱参照
};
