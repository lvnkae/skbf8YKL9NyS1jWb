/*!
 *  @file   holiday_investigator.h
 *  @brief  [common]今日が休日かどうか調査するクラス
 *  @date   2017/12/18
 *  @note   python2.7, boost(boost::python), cpprestsdk に依存している
 *  @note   "yonelabo.com/today_holyday/" が落ちてたらアウト
 *  @note   実行時にholiday_investigator.pyが必要
 */
#pragma once

#include <functional>
#include <string>
#include <memory>

/*!
 */
class HolidayInvestigator
{
public:
    typedef std::function<void(bool b_result, bool is_holiday, const std::wstring& datetime)> CallBackFunction;

    HolidayInvestigator();
    ~HolidayInvestigator();

    /*!
     *  @brief  調査要求を出す
     *  @param  function    調査完了コールバック
     *  @note   すでに調査中だったら無視される
     */
    void Investigate(const CallBackFunction& function);

private:
    HolidayInvestigator(const HolidayInvestigator&);
    HolidayInvestigator& operator= (const HolidayInvestigator&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};
