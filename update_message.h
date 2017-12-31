/*!
 *  @file   update_message.h
 *  @date   2017/12/13
 */
#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

/*!
 */
class UpdateMessage
{
private:
    bool m_b_error;     //!< エラーフラグ
    bool m_b_warning;   //!< 警告フラグ(メッセージのみ)

    std::vector<std::string> m_message;  //!< メッセージ文字列
    std::string m_tab;

public:
    UpdateMessage()
    : m_b_error(false)
    , m_b_warning(false)
    , m_message()
    , m_tab()
    {
    }

    void AddTab()
    {
        m_tab.push_back('\t');
    }
    void DecTab()
    {
        m_tab.pop_back();
    }

    void AddMessage(const std::string& msg)
    {
        m_message.emplace_back(m_tab + msg);
    }
    void AddErrorMessage(const std::string& msg)
    {
        m_b_error = true;
        AddMessage(msg);
    }
    void AddWarningMessage(const std::string& msg)
    {
        m_b_warning = true;
        AddMessage(msg);
    }
    void OutputMessage()
    {
        if (m_b_error || m_b_warning) {
            std::for_each(m_message.begin(), m_message.end(), [](const std::string& str)
            {
                std::cout << str.c_str();
            });
        }
    }
};
