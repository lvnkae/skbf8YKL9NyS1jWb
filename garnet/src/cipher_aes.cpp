/*!
 *  @file   cipher_aes.cpp
 *  @brief  [common]AESで暗号処理するクラス
 *  @date   2017/12/18
 */
#include "cipher_aes.h"

#include "utility_string.h"

#include "aes.h"
#include "dh.h"
#include "modes.h"
#include "osrng.h"
#include <codecvt>

namespace garnet
{

class CipherAES_string::PIMPL
{
private:
    std::string m_key;  //! 暗号鍵
    std::string m_iv;   //! InitialVector
    
    //! フィルタ生成時に渡す暗号化オブジェクト(ローカルはまずそうなので)
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption m_encryption;
    //! 暗号化文字列
    std::string m_enc_text; 
    //! 暗号化フィルタ
    std::unique_ptr<CryptoPP::StreamTransformationFilter> m_pEncryptFilter;

    PIMPL(const PIMPL&);
    PIMPL& operator= (const PIMPL&);

public:
    PIMPL()
    : m_key()
    , m_iv()
    , m_pEncryptFilter()
    {
    }

    /*!
     *  @brief  文字列を暗号化する
     *  @param  rnd_gen 乱数生成器
     *  @param  in_str  暗号化する文字列
     *  @return true    成功
     */
    bool Encrypt(RandomGenerator& rnd_gen, const std::string& src)
    {
        if (!m_key.empty() || m_pEncryptFilter != nullptr) {
            // 暗号化済み
            return false;
        }

        utility_string::GetRandomString(rnd_gen, CryptoPP::AES::DEFAULT_KEYLENGTH, m_key);
        utility_string::GetRandomString(rnd_gen, CryptoPP::AES::BLOCKSIZE, m_iv);

        // 暗号化のための変換フィルタの作成
        m_encryption.SetKeyWithIV(reinterpret_cast<const byte*>(m_key.c_str()), m_key.length(),
                                  reinterpret_cast<const byte*>(m_iv.c_str()), m_iv.length());
        m_pEncryptFilter.reset(new CryptoPP::StreamTransformationFilter(m_encryption, new CryptoPP::StringSink(m_enc_text)));

        // 暗号化
        m_pEncryptFilter->Put(reinterpret_cast<const byte*>(src.c_str()), src.size());
        m_pEncryptFilter->MessageEnd();

        return true;
    }

    /*!
     *  @brief  文字列を復号する
     *  @param[out] dst     復号した文字列の格納先
     *  @return     true    成功
     */
    bool Decrypt(std::string& dst) const
    {
        if (m_key.empty() || nullptr == m_pEncryptFilter) {
            // 暗号化されてない
            return false;
        }

        // 復号化オブジェクトの作成
        CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption dec;
        dec.SetKeyWithIV(reinterpret_cast<const byte*>(m_key.c_str()), m_key.length(),
                         reinterpret_cast<const byte*>(m_iv.c_str()), m_iv.length());
        // 復号化のための変換フィルタの作成
        CryptoPP::StreamTransformationFilter decFilter(dec, new CryptoPP::StringSink(dst));
        decFilter.Put(reinterpret_cast<const byte*>(m_enc_text.c_str()), m_enc_text.size());
        decFilter.MessageEnd();
        
        return true;
    }
};

/*!
 *  @brief
 */
CipherAES_string::CipherAES_string()
: m_pImpl(new PIMPL())
{
}

/*!
 *  @brief
 */
CipherAES_string::~CipherAES_string()
{
}

/*!
 *  @brief  文字列を暗号化する
 *  @param  rnd_gen 乱数生成器
 *  @param  src     暗号化する文字列
 */
bool CipherAES_string::Encrypt(RandomGenerator& rnd_gen, const std::string& src)
{
    return m_pImpl->Encrypt(rnd_gen, src);
}
/*!
 *  @brief  文字列を復号する
 *  @param[out] dst     復号した文字列の格納先
 */
bool CipherAES_string::Decrypt(std::string& dst) const
{
    return m_pImpl->Decrypt(dst);
}
/*!
 *  @brief  文字列を復号する(wstring版)
 *  @param[out] dst     復号した文字列の格納先
 */
bool CipherAES_string::Decrypt(std::wstring& dst) const
{
    std::string str_work;
    bool result = m_pImpl->Decrypt(str_work);
    if (result) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
        dst = std::move(utfconv.from_bytes(str_work));
    }
    return result;
}

} // namespace garnet
