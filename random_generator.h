/*!
 *  @file   random_generator.h
 *  @brief  [common]乱数生成クラス
 *  @date   2017/12/18
 */
#pragma once

#include <memory>

/*!
 *  @brief  "64bit符号なし整数"範囲の一様乱数生成器
 *  @note   ・MTの一様乱数
 *  @note   ・boost::randomはビルド通らないのでstd::randomを使用…
 *  @note   　VS2013における既知の不具合だそうで(https://svn.boost.org/trac10/ticket/11426)
 */
class RandomGenerator
{
public:
    /*!
     *  @param  seed    乱数種
     *  @note   指定なしならハードウェア乱数を採用
     */
     RandomGenerator(size_t seed);
     RandomGenerator();

    ~RandomGenerator();
    /*!
     *  @brief  64bit符号なし整数範囲の一様乱数を得る
     *  @return 乱数
     */
    uint64_t Random();
    /*!
     *  @brief  [a,b]の一様乱数を得る
     *  @param  a   下限
     *  @param  b   上限
     */
    uint64_t Random(uint64_t a, uint64_t b);

private:
    RandomGenerator(const RandomGenerator&);
    RandomGenerator& operator= (const RandomGenerator&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};
