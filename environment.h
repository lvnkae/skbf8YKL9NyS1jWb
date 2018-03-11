/*!
 *  @file   environment.h
 *  @brief  環境設定
 *  @date   2017/12/11
 *  @note   iniファイル仲介
 */
#pragma once

#include "google/google_api_config_fwd.h"
#include "python/python_config_fwd.h"
#include "twitter/twitter_config_fwd.h"

#include <memory>
#include <string>

/*!
 *  @note   singleton
 *  @note   インスタンスは明示的に生成する(呼出側がshared_ptrで保持)
 *  @note   "初回インスタンス取得時生成"ではない
 */
class Environment;
class Environment
{
public:
    /*!
     *  @brief  インスタンス生成 + 初期化
     *  @return インスタンス共有ポインタ
     *  @note   生成済みなら空shared_ptrを返す
     */
    static std::shared_ptr<Environment> Create();
    /*!
     *  @brief  インスタンス取得
     *  @return インスタンス弱参照(const)
     *  @note   生成前に呼ばれたら空weak_ptrを返す
     */
    static std::weak_ptr<const Environment> GetInstance() { return m_pInstance; }

    /*!
     *  @brief  GoogleCalendarAPI設定を得る
     */
    static const garnet::GoogleCalendarAPIConfigRef GetGoogleCarendarAPIConfig();
    /*!
     *  @brief  python設定を得る
     */
    static const garnet::python_config_ref GetPythonConfig();
    /*!
     *  @brief  twitter設定を得る
     */
    static const garnet::twitter_config_ref GetTwitterConfig();

    /*!
     *  @brief  トレーディングスクリプト名を得る
     */
    std::string GetTradingScript() const { return m_trading_script; }

private:
    Environment();
    Environment(const Environment&);
    Environment(Environment&&);
    Environment& operator= (const Environment&);

    /*!
     *  @brief  初期化
     */
    void initialize();

    //! トレーディングスクリプト名
    std::string m_trading_script;
    //! GoogleCalendarAPI設定
    garnet::GoogleCalendarAPIConfigPtr m_google_calendar_api_config;
    //! python設定  
    garnet::python_config_ptr m_python_config;
    //! twitter設定  
    garnet::twitter_config_ptr m_twitter_config;
    //! 自身の弱参照
    static std::weak_ptr<Environment> m_pInstance;
};
