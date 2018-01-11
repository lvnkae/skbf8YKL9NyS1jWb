/*!
 *  @file   random_generator.cpp
 *  @brief  [common]乱数生成クラス
 *  @date   2017/12/18
 */
#include "random_generator.h"

#include <random>

namespace garnet
{

class RandomGenerator::PIMPL
{
private:
    std::mt19937_64 m_mt_generator;

    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

public:
    PIMPL(size_t key)
    : m_mt_generator(key)
    {
    }

    /*!
     *  @brief  64bit符号なし整数範囲の一様乱数を得る
     *  @return 乱数
     */
    uint64_t Random()
    {
        return m_mt_generator();
    }

    /*!
     *  @brief  [a,b]の一様乱数を得る
     *  @param  a   下限
     *  @param  b   上限
     *  @return 乱数
     */
    uint64_t Random(uint64_t a, uint64_t b)
    {
        if (a > b) {
            uint64_t c = b;
            b = a;
            a = c;
        }
        std::uniform_int_distribution<uint64_t> dist(a, b);
        return dist(m_mt_generator);
    }
};

/*!
 *  @param  seed    乱数種
 */
RandomGenerator::RandomGenerator(size_t seed)
: m_pImpl(new PIMPL(seed))
{
}

/*!
*  @brief
*/
RandomGenerator::RandomGenerator()
    : m_pImpl(new PIMPL(std::random_device()()))
{
}

/*!
 *  @brief
 */
RandomGenerator::~RandomGenerator()
{
}

/*!
 *  @brief  32bit符号なし整数範囲の一様乱数を得る
 */
uint64_t RandomGenerator::Random()
{
    return m_pImpl->Random();
}

/*!
 *  @brief  [a,b]の一様乱数を得る
 *  @param  a   下限
 *  @param  b   上限
 */
uint64_t RandomGenerator::Random(uint64_t a, uint64_t b)
{
    return m_pImpl->Random(a, b);
}

} // namespace garnet
