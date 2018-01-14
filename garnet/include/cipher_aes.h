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
 *  @brief  文字列をAESで暗号化して保持するクラス
 *  @note   1文字列につき1インスタンス
 *  @note    暗号化は1インスタンスで1回だけ
 *  @note    復号(元の文字列取り出し)は何度でもOK
 *  @note   暗号鍵とivは中で勝手に作る(参照不可)
 */
class CipherAES_string
{
public:
    CipherAES_string();
    ~CipherAES_string();

    /*!
     *  @brief  文字列を暗号化する
     *  @param  rnd_gen 乱数生成器
     *  @param  src     暗号化する文字列(utf-8)
     *  @retval true    成功
     */
    bool Encrypt(RandomGenerator& rnd_gen, const std::string& src);
    /*!
     *  @brief  文字列を復号する
     *  @param[out] dst     復号した文字列の格納先(utf-8)
     *  @retval     true    成功
     */
    bool Decrypt(std::string& dst) const;
    /*!
     *  @brief  文字列を復号する(wstring版)
     *  @param[out] dst     復号した文字列の格納先(utf-16)
     *  @retval     true    成功
     */
    bool Decrypt(std::wstring& dst) const;


private:
    CipherAES_string(const CipherAES_string&);
    CipherAES_string(CipherAES_string&&);
    CipherAES_string& operator= (const CipherAES_string&);

    class PIMPL;
    std::unique_ptr<PIMPL> m_pImpl;
};

} // namespace garnet
