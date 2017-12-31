/*!
 *  @file   yymmdd.h
 *  @brief  [common]年月日だけの最少時間構成
 *  @date   2017/12/30
 */
#pragma once

/*!
 *  @brief  月日
 */
struct MMDD
{
    int32_t m_month;    //! 月(1始まり)
    int32_t m_day;      //! 日(1始まり)

    MMDD()
    : m_month(0)
    , m_day(0)
    {
    }

    MMDD(int32_t m, int32_t d)
    : m_month(m)
    , m_day(d)
    {
    }

    bool operator==(const MMDD& right) const
    {
        return (m_month == right.m_month && m_day == right.m_day);
    }
};

/*!
 *  @brief  年月日
 */
struct YYMMDD : public MMDD
{
    int32_t m_year; //!< 年(西暦)

    YYMMDD()
    : MMDD()
    , m_year(0)
    {
    }

    YYMMDD(int32_t y, int32_t m, int32_t d)
    : MMDD(m, d)
    , m_year(y)
    {
    }

    bool operator==(const YYMMDD& right) const
    {
        if (MMDD::operator==(right)) {
            return m_year == right.m_year;
        } else {
            return false;
        }
    }
};
