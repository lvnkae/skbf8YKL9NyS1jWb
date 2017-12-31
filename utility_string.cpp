/*!
 *  @file   utility_string.cpp
 *  @brief  [common]•¶š—ñ‘€ìUtility
 *  @date   2017/12/18
 */
#include "utility_string.h"

#include "random_generator.h"
#include <vector>

namespace utility
{

/*!
 *  @brief  ƒ‰ƒ“ƒ_ƒ€‚ÈASCII•¶š—ñ‚ğ“¾‚é
 *  @param[in]  rnd_gen —”¶¬Ší
 *  @param[in]  len     •¶š”
 *  @param[out] o_str   Ši”[æ
 */
void GetRandomString(RandomGenerator& rnd_gen, size_t len, std::string& o_string)
{
    if (len == 0) {
        return; // •s³
    }

    const int32_t USE_ASCII_NUMBERS = 10;
    const int32_t USE_ASCII_LETTERS = 26;
    const int32_t USE_ASCII_ALLS = USE_ASCII_NUMBERS + USE_ASCII_LETTERS * 2;

    std::vector<uint8_t> ascii_array;
    ascii_array.reserve(USE_ASCII_ALLS);
    for (int32_t inx = 0; inx < USE_ASCII_NUMBERS; inx++) {
        ascii_array.push_back('0' + inx);
    }
    for (int32_t inx = 0; inx < USE_ASCII_LETTERS; inx++) {
        ascii_array.push_back('A' + inx);
    }
    for (int32_t inx = 0; inx < USE_ASCII_LETTERS; inx++) {
        ascii_array.push_back('a' + inx);
    }

    o_string.reserve(len);
    for (size_t inx = 0; inx < len; inx++) {
        o_string.push_back(ascii_array[rnd_gen.Random(0, USE_ASCII_ALLS - 1)]);
    }
}


/*!
 *  @brief  ‘å•¶š¨¬•¶š•ÏŠ·
 *  @param[in]  src
 *  @param[out] dst
 */
template<typename SrcString, typename SrcChar>
void ToLowerCore(const SrcString& src, std::string& dst)
{
    const char DIFF_LOWER_AND_UPPER = ('a' - 'A');

    dst.reserve(src.size());
    for (const SrcChar c: src) {
        if (static_cast<SrcChar>('A') <= c && c <= static_cast<SrcChar>('Z')) {
            dst.push_back(static_cast<char>(c)+DIFF_LOWER_AND_UPPER);
        } else if (static_cast<SrcChar>(' ') <= c && c <= static_cast<SrcChar>('~')) {
            dst.push_back(static_cast<char>(c));
        }
    }
}
void ToLower(const std::wstring& src, std::string& dst) { ToLowerCore<std::wstring, wchar_t>(src, dst); }
void ToLower(const std::string& src, std::string& dst) { ToLowerCore<std::string, char>(src, dst); }

/*!
 *  @brief  ”{¸“x•‚“®¬”‚ğ¬”“_ˆÈ‰ºNˆÊ‚Ü‚Å•¶š—ñ‚É•ÏŠ·
 *  @param[in]  src
 *  @param[out] dst Ši”[æ
 */
std::wstring ToWstringOrder(float64 src, uint32_t order)
{
    std::wstring dst(std::to_wstring(src));
    auto pos = dst.find(L'.');
    if (pos != std::wstring::npos) {
        std::wstring::iterator it = dst.begin();
        if (order == 0) {
            it = it + pos;  // 0ˆÊw’è‚È‚ç'.'‚àÁ‚·
        } else {
            std::wstring::size_type del_point = pos + 1 + order;
            if (dst.size() <= del_point) {
                return dst; // Á‚·•K—v‚È‚µ
            }
            it = it + del_point;
        }
        dst.erase(it, dst.end());
    }
    return dst;
}


} // namespace utility
