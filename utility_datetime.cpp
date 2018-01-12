/*!
 *  @file   utility_datetime.cpp
 *  @brief  [common]日時関連Utility
 *  @date   2017/12/19
 */
#include "utility_datetime.h"

#include "hhmmss.h"
#include "yymmdd.h"
#include "garnet_time.h"

#include <chrono>
#include <codecvt>
#include <ctime>
#include <sstream>
#include "boost/date_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"

namespace
{
const int32_t DAYS_OF_1WEEK = 7;                // 一週間=7日
const int32_t HOURS_OF_1DAY = 24;               // 1日=24時間
const int32_t MINUTES_OF_1HOUR = 60;            // 1時間=60分
const int32_t SECONDS_OF_1MINUTE = 60;          // 1分=60秒
const int64_t MILISECONDS_OF_1SECOND = 1000;    // 1秒=100ミリ秒

// 1時間の秒数
const int32_t SECONDS_OF_1HOUR = MINUTES_OF_1HOUR*SECONDS_OF_1MINUTE;
// 1日の秒数
const int32_t SECONDS_OF_1DAY = HOURS_OF_1DAY*MINUTES_OF_1HOUR*SECONDS_OF_1MINUTE;
}

namespace garnet
{
namespace utility_datetime
{

/*!
 *  @brief  RFC1123形式の日時文字列からローカル日時を得る
 *  @param[in]  rfc1123 RFC1123形式日時文字列
 *  @param[out] o_lcdt  ローカル日時格納先
 */
boost::posix_time::ptime ToLocalTimeFromRFC1123(const std::wstring& rfc1123)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
    return ToLocalTimeFromRFC1123(cv.to_bytes(rfc1123));
}

/*!
 *  @brief  RFC1123形式の日時文字列からローカル日時を得る
 *  @param[in]  rfc1123 RFC1123形式日時文字列
 *  @param[out] o_lcdt  ローカル日時格納先
 */
boost::posix_time::ptime ToLocalTimeFromRFC1123(const std::string& rfc1123)
{
    // newしたfacetはストリーム内で解放される(らしい)
    boost::local_time::local_time_input_facet* inputFacet
        = new boost::local_time::local_time_input_facet();
    std::stringstream ss;
    ss.imbue(std::locale(ss.getloc(), inputFacet));
    ss.str(rfc1123);
    inputFacet->format("%a, %d %b %Y %H:%M:%S %ZP");

    // ストリームからldtに変換すると設定したzone_ptrが潰されてしまうので
    // 変換したldtからはUTCだけを得て、別途用意したlocalのtime_zoneからオフセットを得る
    // (正しい使い方と思われるやり方は全部駄目だった)
    boost::local_time::local_date_time ldt(
        boost::local_time::local_date_time::utc_time_type(), nullptr);
    ss >> ldt;
    boost::local_time::time_zone_ptr local_zone(new boost::local_time::posix_time_zone("GMT+09"));
    return ldt.utc_time() + local_zone->base_utc_offset() + local_zone->dst_offset();
}


/*!
 *  @brief  指定フォーマットの日時文字列をtmに変換
 *  @param[in]  src     日時文字列
 *  @param[in]  format  ↑形式 (%H:%M:%S など)
 *  @param[out] o_tm    格納先
 *  @note   ローカル変換なし
 */
bool ToTimeFromString(const std::string& src, const std::string& format, garnet::sTime& o_tm)
{
    std::tm t_tm;
    std::istringstream ss(src);
    ss >> std::get_time(&t_tm, format.c_str());
    o_tm = std::move(t_tm);

    return !ss.fail();
}
/*!
 *  @brief  ptimeからtmに変換
 *  @param[in]  src     ptime
 *  @param[out] o_tm    格納先
 *  @param[in]  tm::tm_isdstは常にfalse(サマータイム考慮しない)
 */
void ToTimeFromBoostPosixTime(const boost::posix_time::ptime& src, garnet::sTime& o_tm)
{
    boost::gregorian::date d = src.date();
    boost::posix_time::time_duration t = src.time_of_day();

    o_tm.tm_sec = t.seconds();
    o_tm.tm_min = t.minutes();
    o_tm.tm_hour = t.hours();
    o_tm.tm_mday = d.day();
    o_tm.tm_mon  = d.month() - 1;
    o_tm.tm_year = d.year() - 1900;
    o_tm.tm_wday = d.day_of_week().as_number();
    o_tm.tm_yday = d.day_of_year();
    o_tm.tm_isdst = false;
}


/*!
 *  @brief  経過時間を得る
 *  @return tickCount(ミリ秒)
 */
int64_t GetTickCountGeneral()
{
    auto time_point = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}


/*!
 *  @brief  srcとシステムローカル時間との差を秒[絶対値]で得る
 *  @param  src 比べる時間
 *  @return 秒差
 */
uint32_t GetDiffSecondsFromLocalMachineTime(const garnet::sTime& src)
{
    std::tm src_tm;
    src.copy(src_tm);
    time_t tt_lc = std::time(nullptr);
    time_t tt_src = std::mktime(&src_tm);
    int32_t tt_dif = static_cast<int32_t>(tt_lc - tt_src);
    return std::abs(tt_dif);
}
/*!
 *  @brief  システムローカル時間を文字列で得る
 *  @return 文字列
 */
std::wstring GetLocalMachineTime(const std::wstring& format)
{
    std::wostringstream ss;

    const time_t tt_lc = std::time(nullptr);
    std::tm tm_lc;
#if defined(_WINDOWS)
    localtime_s(&tm_lc, &tt_lc);
#else
    tm_lc = *std::localtime(&tt_lc);
#endif/* defined(_WINDOWS) */
    ss << std::put_time(&tm_lc, format.c_str());

    return ss.str();
}


/*!
 *  @brief  base_tmにdiff_msを足しo_nowに出力
 *  @param[in]  base_tm ベース時刻
 *  @param[in]  diff_ms 差分ミリ秒
 *  @param[out] o_now   格納先
 */
void AddTimeAndDiffMS(const garnet::sTime& base_tm, int64_t diff_ms, garnet::sTime& o_now)
{
    std::tm base_tm_cpy;
    base_tm.copy(base_tm_cpy);
    time_t tt_base = std::mktime(&base_tm_cpy);
    time_t tt_after = tt_base + static_cast<time_t>(diff_ms/MILISECONDS_OF_1SECOND);
#if defined(_WINDOWS)
    std::tm o_tm;
    localtime_s(&o_tm, &tt_after);
    o_now = std::move(o_tm);
#else
    o_now = *std::localtime(&tt_after);
#endif/* defined(_WINDOWS) */
}


/*!
 *  @brief  指定日時after_day後の00:00までの時間をミリ秒で得る
 *  @param  pt          boost時間インターフェイス
 *  @param  after_day   何日後か
 *  @note   mktimeもlocaltimeもローカルタイム処理
 *  @note   localeは未設定でもJSTかGMTかUCTしかありえず、どれが来ても差分を取る分には辻褄合うのでOK
 */
int64_t GetAfterDayLimitMS(const boost::posix_time::ptime& pt, int32_t after_day)
{
    if (after_day <= 0) {
        return 0;
    }
    garnet::sTime src_time;
    ToTimeFromBoostPosixTime(pt, src_time);
    std::tm src_tm;
    src_time.copy(src_tm);
    std::time_t src_tt = std::mktime(&src_tm);
    std::time_t after_tt = src_tt + SECONDS_OF_1DAY*after_day;
#if defined(_WINDOWS)
    std::tm after_tm;
    localtime_s(&after_tm, &after_tt);
#else
    std::tm after_tm = *std::localtime(&after_tt);
#endif/* defined(_WINDOWS) */
    {
        after_tm.tm_sec = 0;
        after_tm.tm_min = 0;
        after_tm.tm_hour = 0;
        after_tm.tm_wday = (src_tm.tm_wday+after_day)%DAYS_OF_1WEEK;
        after_tt = std::mktime(&after_tm);
    }

    return ToMiliSecondsFromSecond(static_cast<int32_t>(after_tt - src_tt));
}
/*!
 *  @brief  分をミリ秒で得る
 *  @param  minute  任意時間[分]
 */
int64_t ToMiliSecondsFromMinute(int32_t minute)
{
    return static_cast<int64_t>(minute)*ToMiliSecondsFromSecond(SECONDS_OF_1MINUTE);
}
/*!
 *  @brief  秒をミリ秒で得る
*  @param  second  任意時間[秒]
  */
int64_t ToMiliSecondsFromSecond(int32_t second)
{
    return static_cast<int64_t>(second)*MILISECONDS_OF_1SECOND;
}

} // namespace utility_datetime


/*!
 *  @param  stime   年月日時分秒パラメータ
 */
HHMMSS::HHMMSS(const garnet::sTime& stime)
: m_hour(stime.tm_hour)
, m_minute(stime.tm_min)
, m_second(stime.tm_sec)
{
}
HHMMSS::HHMMSS(garnet::sTime&& stime)
: m_hour(stime.tm_hour)
, m_minute(stime.tm_min)
, m_second(stime.tm_sec)
{
}
/*!
 *  @brief  00:00:00からの経過秒数を得る
 */
int32_t HHMMSS::GetPastSecond() const
{
    return SECONDS_OF_1HOUR*m_hour + SECONDS_OF_1MINUTE*m_minute + m_second;
}

/*!
 *  @param  tm  年月日時分秒パラメータ
 */
MMDD::MMDD(const garnet::sTime& stime)
: m_month(stime.tm_mon+1) // 1始まり
, m_day(stime.tm_mday)
{
}
MMDD::MMDD(garnet::sTime&& stime)
: m_month(stime.tm_mon+1) // 1始まり
, m_day(stime.tm_mday)
{
}
/*!
 *  @param  src "MM/DD"形式の月日文字列
 */
MMDD MMDD::Create(const std::string& src)
{
    garnet::sTime mmdd_tm;
    if (utility_datetime::ToTimeFromString(src, "%m/%d", mmdd_tm)) {
        return MMDD(mmdd_tm);
    } else {
        return MMDD();
    }
}

/*!
 *  @param  tm  年月日時分秒パラメータ
 */
YYMMDD::YYMMDD(const garnet::sTime& stime)
: MMDD(stime)
, m_year(stime.tm_year + 1900) // 西暦
{
}
YYMMDD::YYMMDD(garnet::sTime&& stime)
: MMDD(stime)
, m_year(stime.tm_year + 1900) // 西暦
{
}
/*!
 *  @param  src "YYYY/MM/DD"形式の年月日文字列
 */
YYMMDD YYMMDD::Create(const std::string& src)
{
    garnet::sTime yymmdd_tm;
    if (utility_datetime::ToTimeFromString(src, "%Y/%m/%d", yymmdd_tm)) {
        if (src.find('/') < 4) {
            yymmdd_tm.tm_year += 100;
        }
        return YYMMDD(yymmdd_tm);
    } else {
        return YYMMDD();
    }
}

sTime::sTime(const std::tm& src)
: tm_sec(src.tm_sec)
, tm_min(src.tm_min)
, tm_hour(src.tm_hour)
, tm_mday(src.tm_mday)
, tm_mon(src.tm_mon)
, tm_year(src.tm_year)
, tm_wday(src.tm_wday)
, tm_yday(src.tm_yday)
, tm_isdst(src.tm_isdst)
{
    static_assert(sizeof(std::tm)==sizeof(garnet::sTime), "");
}

sTime::sTime(std::tm&& src)
: tm_sec(src.tm_sec)
, tm_min(src.tm_min)
, tm_hour(src.tm_hour)
, tm_mday(src.tm_mday)
, tm_mon(src.tm_mon)
, tm_year(src.tm_year)
, tm_wday(src.tm_wday)
, tm_yday(src.tm_yday)
, tm_isdst(src.tm_isdst)
{
}

void sTime::copy(std::tm& dst) const
{
    dst.tm_sec = tm_sec;
    dst.tm_min = tm_min;
    dst.tm_hour = tm_hour;
    dst.tm_mday = tm_mday;
    dst.tm_mon = tm_mon;
    dst.tm_year = tm_year;
    dst.tm_wday = tm_wday;
    dst.tm_yday = tm_yday;
    dst.tm_isdst = tm_isdst;
}

} // namespace garnet
