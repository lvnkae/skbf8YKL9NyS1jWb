/*!
 *  @file   cipher_aes.h
 *  @brief  [common]AESで暗号処理するクラス
 *  @date   2017/12/18
 *  @note   Crypto++, RandomGenerator, utility_string に依存している
 */
#pragma once

#include <string>
#include <memory>

namespace garnet
{

class RandomGenerator;

/*!
 *  @note   ・使い捨てなので1クラスで1回しか暗号化できない(復号は何度でもOK)
 *  @note   ・暗号鍵とivは中で勝手に作る
 */
class CipherAES
{
public:
    CipherAES();
    ~CipherAES();

    /*!
     *  @brief  文字列を暗号化する
     *  @param  rnd_gen 乱数生成器
     *  @param  src     暗号化する文字列
     *  @retval true    成功
     */
    bool Encrypt(RandomGenerator& rnd_gen, const std::string& src);
    /*!
     *  @brief  文字列を復号する
     *  @param[out] dst     復号した文字列の格納先
     *  @retval     true    成功
     */
    bool Decrypt(std::string& dst) const;
    /*!
     *  @brief  文字列を復号する(wstring版)
     *  @param[out] dst     復号した文字列の格納先
     *  @retval     true    成功
     */
    bool Decrypt(std::wstring& dst) const;


private:
    CipherAES(const CipherAES&);
    CipherAES& operator= (const CipherAES&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace garnet
