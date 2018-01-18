/*!
 *  @file   holiday_investigator.h
 *  @brief  [common]�������x�����ǂ�����������N���X
 *  @date   2017/12/18
 *  @note   python2.7, boost(boost::python), cpprestsdk �Ɉˑ����Ă���
 *  @note   "yonelabo.com/today_holyday/" �������Ă���A�E�g
 *  @note   ���s����holiday_investigator.py���K�v
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
     *  @brief  �����v�����o��
     *  @param  function    ���������R�[���o�b�N
     *  @note   ���łɒ������������疳�������
     */
    void Investigate(const CallBackFunction& function);

private:
    HolidayInvestigator(const HolidayInvestigator&);
    HolidayInvestigator& operator= (const HolidayInvestigator&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};
