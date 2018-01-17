/*!
 *  @file   stock_portfolio.h
 *  @brief  ���Ď������f�[�^
 *  @date   2017/12/24
 */
#pragma once

#include "stock_code.h"
#include "hhmmss.h"
#include "yymmdd.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace trading
{
struct RcvStockValueData;

/*!
 *  @brief  �������i�f�[�^
 *  @note   �C�ӊ��Ԃ̉��i�Əo�������W�ς�������
 */
struct StockValueData
{
    struct stockValue
    {
        garnet::HHMMSS m_hhmmss;//!< �����b
        float64 m_value;        //!< ���i
        int64_t m_volume;       //!< �o����

        stockValue()
        : m_hhmmss()
        , m_value(0.f)
        , m_volume(0)
        {
        }

        stockValue(const garnet::sTime& tm, float64 value, int64_t volume)
        : m_hhmmss(tm)
        , m_value(value)
        , m_volume(volume)
        {
        }
    };

    StockCode m_code;   //!< �����R�[�h
    float64 m_open;     //!< �n�l
    float64 m_high;     //!< ���l
    float64 m_low;      //!< ���l
    float64 m_close;    //!< �O�c�Ɠ��I�l
    std::vector<stockValue> m_value_data;   //!< ���n�񉿊i�f�[�^�Q

    StockValueData()
    : m_code()
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_value_data()
    {
    }
    StockValueData(uint32_t scode)
    : m_code(scode)
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_value_data()
    {
    }

    /*!
     *  @brief  ���i�f�[�^�X�V
     *  @param  src     ���i�f�[�^
     *  @param  date    �擾����(�T�[�o���Ԃ��g��)
     */
    void UpdateValueData(const RcvStockValueData& src, const garnet::sTime& date);

    /*!
     *  @brief  ���O�o��
     *  @param  filename    �o�̓t�@�C����(�p�X�܂�)
     */
    void OutputLog(const std::string& filename) const;
};

/*!
 *  @brief  ���i�f�[�^(1������)��M�`��
 *  @note   ����u�Ԃ̊��f�[�^
 */
struct RcvStockValueData
{
    uint32_t m_code;    //!< �����R�[�h
    float64 m_value;    //!< ���l
    float64 m_open;     //!< �n�l
    float64 m_high;     //!< ���l
    float64 m_low;      //!< ���l
    float64 m_close;    //!< �O�c�Ɠ��I�l    
    int64_t m_volume;   //!< �o����

    RcvStockValueData()
    : m_code(0)
    , m_value(0.f)
    , m_open(0.f)
    , m_high(0.f)
    , m_low(0.f)
    , m_close(0.f)
    , m_volume(0)
    {
    }
};

} // namespace trading
