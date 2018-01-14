/*!
 *  @file   hhmmss.h
 *  @brief  [common]•ª•b‚¾‚¯‚ÌÅ­ŠÔ\¬
 *  @date   2017/12/27
 */
#pragma once

namespace garnet
{
struct sTime;

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

    HHMMSS(const garnet::sTime&);
    HHMMSS(garnet::sTime&&);

    /*!
     *  @brief  00:00:00‚©‚ç‚ÌŒo‰ß•b”‚ğ“¾‚é
     */
    int32_t GetPastSecond() const;
    /*!
     *  @brief  ˆø‚«Z
     *  @return this-right‚ğ•b’PˆÊ‚Å•Ô‚·
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
