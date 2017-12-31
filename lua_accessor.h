/*!
 *  @file   lua_accessor.h
 *  @brief  [common]luaとのやりとり仲介
 *  @date   2017/05/04
 *  @note   luaに依存している
 */
#pragma once

#include <memory>
#include <stack>
#include <string>

struct lua_State;
class LuaAccessor;

/*!
 *  @brief  lua仲介クラス
 */
class LuaAccessor
{
public:
    LuaAccessor();
    ~LuaAccessor();

    /*!
     *  @brief  ファイル読み込み
     *  @param  file_name   ファイル名フルパス
     *  @retval true    成功
     *  @note   完了復帰
     */
    bool DoFile(const std::string& file_name);
    /*!
     *  @brief  luaスタックを全クリアする
     */
    void ClearStack();

    /*!
     *  @brief  親テーブルを開く
     *  @param  table_name  テーブル名
     *  @return テーブル要素数
     */
    int32_t OpenTable(const std::string& table_name);
    /*!
     *  @brief  子テーブルを開く
     *  @param  table_name  子テーブル名
     *  @return 子テーブル要素数
     */
    int32_t OpenChildTable(const std::string& table_name);
    /*!
     *  @brief  子テーブルを開く
     *  @param  table_inx    子テーブルインデックス(0始まり)
     *  @return 子テーブル要素数
     */
    int32_t OpenChildTable(int32_t table_inx);
    /*!
     *  @brief  最後に開いたテーブルをクローズする
     */
    void CloseTable();

    /*!
     *  @brief  グローバルパラメータ取得
     *  @param[in]  param_name  パラメータ名
     *  @param[out] o_param     パラメータ格納先
     *  @retval     true        成功
     */
    bool GetGlobalParam(const std::string& param_name, std::string& o_param);
    bool GetGlobalParam(const std::string& param_name, int32_t& o_param);
    /*!
     *  @brief  テーブルパラメータ取得
     *  @param[in]  param_name  パラメータ名
     *  @param[out] o_param     パラメータ格納先
     *  @retval     true        成功
     */
    bool GetTableParam(const std::string& param_name, std::string& o_param);
    bool GetTableParam(const std::string& param_name, int32_t& o_param);
    bool GetTableParam(const std::string& param_name, float32& o_param);
    bool GetTableParam(const std::string& param_name, bool& o_param);
    /*!
     *  @brief  配列パラメータ取得
     *  @param[in]  param_inx   パラメータインックス(0始まり)
     *  @param[out] o_param     パラメータ格納先
     *  @retval     true        成功
     */
    bool GetArrayParam(int32_t param_inx, std::string& o_param);
    bool GetArrayParam(int32_t param_inx, int32_t& o_param);

    /*!
     *  @brief  lua関数リファレンス取得
     *  @param[in]  param_name  パラメータ名(lua関数の格納されたパラメータの名前)
     *  @param[out] o_ref       関数リファレンス格納先
     *  @retval     true        成功
     */
    bool GetLuaFunctionReference(const std::string& param_name, int32_t& o_ref);
    /*!
     *  @brief  lua関数呼び出し
     *  @param  func_ref    lua関数リファレンス
     *  @param              lua関数引数群
     *  @return 浮動小数点パラメータ
     */
    bool CallLuaBoolFunction(int32_t func_ref, float64, float64, float64, float64);
    float64 CallLuaFloatFunction(int32_t func_ref, float64, float64, float64, float64);

private:
    LuaAccessor(const LuaAccessor&);
    LuaAccessor& operator= (const LuaAccessor&);

    //!< Luaステート(専用の終了処理を通す必要があるので生ポ…)
    lua_State* m_pState;
    //!< Luaテーブルを開いた時のスタック位置
    std::stack<int32_t> m_table_top_stack;
};
