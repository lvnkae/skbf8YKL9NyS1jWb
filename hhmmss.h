/*!
 *  @file   hhmmss.h
 *  @brief  [common]時分秒だけの最少時間構成
 *  @date   2017/12/27
 */
#pragma once

#include <ctime>

namespace garnet
{

struct HHMMSS
{
    int32_t m_hour;
    int32_t m_minute;
    int32_t m_second;

    HHMMSS()
    : m_hour(0)
    , m_minute(0)
    , m_second(0)
    {
    }

    HHMMSS(int32_t h, int32_t m, int32_t s)
    : m_hour(h)
    , m_minute(m)
    , m_second(s)
    {
    }

    HHMMSS(const std::tm&);

    /*!
     *  @brief  00:00:00からの経過秒数を得る
     */
    int32_t GetPastSecond() const;
    /*!
     *  @brief  引き算
     *  @return this-rightを秒単位で返す
     */
    int32_t Sub(const HHMMSS& right) const
    {
        return GetPastSecond() - right.GetPastSecond();
    }
    bool operator<(const HHMMSS& right) const
    {
        return GetPastSecond() < right.GetPastSecond();
    }
    bool operator>(const HHMMSS& right) const
    {
        return GetPastSecond() > right.GetPastSecond();
    }
};

} // namespace garnet
