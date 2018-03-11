/*!
 *  @file   environment.h
 *  @brief  ���ݒ�
 *  @date   2017/12/11
 *  @note   ini�t�@�C������
 */
#pragma once

#include "google/google_api_config_fwd.h"
#include "python/python_config_fwd.h"
#include "twitter/twitter_config_fwd.h"

#include <memory>
#include <string>

/*!
 *  @note   singleton
 *  @note   �C���X�^���X�͖����I�ɐ�������(�ďo����shared_ptr�ŕێ�)
 *  @note   "����C���X�^���X�擾������"�ł͂Ȃ�
 */
class Environment;
class Environment
{
public:
    /*!
     *  @brief  �C���X�^���X���� + ������
     *  @return �C���X�^���X���L�|�C���^
     *  @note   �����ς݂Ȃ��shared_ptr��Ԃ�
     */
    static std::shared_ptr<Environment> Create();
    /*!
     *  @brief  �C���X�^���X�擾
     *  @return �C���X�^���X��Q��(const)
     *  @note   �����O�ɌĂ΂ꂽ���weak_ptr��Ԃ�
     */
    static std::weak_ptr<const Environment> GetInstance() { return m_pInstance; }

    /*!
     *  @brief  GoogleCalendarAPI�ݒ�𓾂�
     */
    static const garnet::GoogleCalendarAPIConfigRef GetGoogleCarendarAPIConfig();
    /*!
     *  @brief  python�ݒ�𓾂�
     */
    static const garnet::python_config_ref GetPythonConfig();
    /*!
     *  @brief  twitter�ݒ�𓾂�
     */
    static const garnet::twitter_config_ref GetTwitterConfig();

    /*!
     *  @brief  �g���[�f�B���O�X�N���v�g���𓾂�
     */
    std::string GetTradingScript() const { return m_trading_script; }

private:
    Environment();
    Environment(const Environment&);
    Environment(Environment&&);
    Environment& operator= (const Environment&);

    /*!
     *  @brief  ������
     */
    void initialize();

    //! �g���[�f�B���O�X�N���v�g��
    std::string m_trading_script;
    //! GoogleCalendarAPI�ݒ�
    garnet::GoogleCalendarAPIConfigPtr m_google_calendar_api_config;
    //! python�ݒ�  
    garnet::python_config_ptr m_python_config;
    //! twitter�ݒ�  
    garnet::twitter_config_ptr m_twitter_config;
    //! ���g�̎�Q��
    static std::weak_ptr<Environment> m_pInstance;
};
