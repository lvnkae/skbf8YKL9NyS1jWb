/*!
 *  @file   yymmdd.h
 *  @brief  [common]年月日だけの最少時間構成
 *  @date   2017/12/30
 */
#pragma once

#include <string>

namespace garnet
{
struct sTime;

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

    MMDD(const garnet::sTime&);
    MMDD(garnet::sTime&&);

    /*!
     *  @brief  "MM/DD"形式の月日文字列から生成
     *  @param  src 月日文字列
     */
    static MMDD Create(const std::string& src);

    bool operator==(const MMDD& right) const
    {
        return (m_month == right.m_month && m_day == right.m_day);
    }

    bool empty() const
    {
        return m_month == 0; // "0月"は引数なし生成時の初期化値なので空とみなす
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

    YYMMDD(const garnet::sTime&);
    YYMMDD(garnet::sTime&&);

    /*!
     *  @brief  "YYYY/MM/DD"形式の年月日文字列から生成
     *  @param  src 年月日文字列(1900年以降)
     *  @note   西暦部分がが3桁以下ならば2000年代の省略形とみなす
     *  @note   ("10/01/01"は2010年1月1日)
     */
    static YYMMDD Create(const std::string& src);

    bool operator==(const YYMMDD& right) const
    {
        if (MMDD::operator==(right)) {
            return m_year == right.m_year;
        } else {
            return false;
        }
    }
};

} // namespace garnet
