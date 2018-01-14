/*!
 *  @file   lua_accessor.cpp
 *  @brief  [common]luaとのやりとり仲介
 *  @date   2017/05/04
 */
#include "lua_accessor.h"
#include "lua.hpp"

namespace garnet
{
namespace LuaAccessorLocal
{

const int32_t STACKTOP = -1;

/*!
 *  @brief  テーブルパラメータ取得(Core)
 *  @param[in]  param_name  パラメータ名
 *  @param[out] o_param     パラメータ格納先
 *  @retval     true        成功
 *  @note   開いた(スタックに積まれた)状態のテーブル内パラメータ取得
 */
bool GetTableParamCore(lua_State* state_ptr, const std::string& param_name, std::string& o_param)
{
    if (0 == lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str())) {
        return false;
    }
    if (!lua_isstring(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = lua_tostring(state_ptr, LuaAccessorLocal::STACKTOP);
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const std::string& param_name, int32_t& o_param)
{
    if (0 == lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str())) {
        return false;
    }
    if (!lua_isinteger(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = static_cast<int32_t>(lua_tointeger(state_ptr, LuaAccessorLocal::STACKTOP));
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const std::string& param_name, float32& o_param)
{
    if (0 == lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str())) {
        return false;
    }
    if (!lua_isnumber(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = static_cast<float32>(lua_tonumber(state_ptr, LuaAccessorLocal::STACKTOP));
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const std::string& param_name, float64& o_param)
{
    if (0 == lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str())) {
        return false;
    }
    if (!lua_isnumber(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = static_cast<float64>(lua_tonumber(state_ptr, LuaAccessorLocal::STACKTOP));
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const std::string& param_name, bool& o_param)
{
    if (0 == lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str())) {
        return false;
    }
    if (!lua_isboolean(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = (lua_toboolean(state_ptr, LuaAccessorLocal::STACKTOP)) ? true : false;
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const int32_t& param_inx, std::string& o_param)
{
    int32_t lua_param_inx = param_inx + 1;
    if (0 == lua_geti(state_ptr, LuaAccessorLocal::STACKTOP, lua_param_inx)) {
        return false;
    }
    if (!lua_isstring(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = lua_tostring(state_ptr, LuaAccessorLocal::STACKTOP);
    return true;
}
bool GetTableParamCore(lua_State* state_ptr, const int32_t& param_inx, int32_t& o_param)
{
    int32_t lua_param_inx = param_inx + 1;
    if (0 == lua_geti(state_ptr, LuaAccessorLocal::STACKTOP, lua_param_inx)) {
        return false;
    }
    if (!lua_isinteger(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = static_cast<int32_t>(lua_tointeger(state_ptr, LuaAccessorLocal::STACKTOP));
    return true;
}

/*!
 *  @brief  テーブルパラメータ取得ラップ関数
 *  @param[in]  param_name  パラメータ名
 *  @param[out] o_param     パラメータ格納先
 *  @retval     true        成功
 *  @note   "テーブルを開く＝スタックのトップにテーブルを積む"
 *  @note   スタックに積んだパラメータを捨てておかないとテーブルがトップでなくなってしまう
 */
template<typename Tname, typename Tout>
bool GetTableParamWrapper(lua_State* state_ptr, const Tname& param_name, Tout& o_param)
{
    int32_t prev_stack_count = lua_gettop(state_ptr);
    bool result = LuaAccessorLocal::GetTableParamCore(state_ptr, param_name, o_param);
    lua_pop(state_ptr, lua_gettop(state_ptr) - prev_stack_count);
    return result;
}

/*!
 *  @brief  lua関数呼び出し
 *  @param  state_ptr   luaステート
 *  @param  func_ref    lua関数リファレンス
 *  @param  args        lua関数引数群
 */
template<typename... T>
bool CallLuaBoolFunction(lua_State* state_ptr, int32_t func_ref, T... args)
{
    int32_t prev_stack_count = lua_gettop(state_ptr);

    // 関数をスタックに積む
    lua_rawgeti(state_ptr, LUA_REGISTRYINDEX, func_ref);
    // 引数をスタックに積む
    for (auto const& argument : { args... }) {
        lua_pushnumber(state_ptr, argument);
    }
    // 関数コール
    const int NUM_ARGS = static_cast<int>(sizeof...(args));
    const int NUM_RETVAL = 1;
    int ret = lua_pcall(state_ptr, NUM_ARGS, NUM_RETVAL, 0);
    // スタックから戻り値を得る
    bool retval = false;
    if (ret == LUA_OK && lua_isboolean(state_ptr, LuaAccessorLocal::STACKTOP)) {
        retval = (lua_toboolean(state_ptr, LuaAccessorLocal::STACKTOP)) ?true :false;
    }

    lua_pop(state_ptr, lua_gettop(state_ptr) - prev_stack_count);
    return retval;
}
template<typename RT, typename... T>
RT CallLuaValueFunction(lua_State* state_ptr, int32_t func_ref, T... args)
{
    int32_t prev_stack_count = lua_gettop(state_ptr);

    // 関数をスタックに積む
    lua_rawgeti(state_ptr, LUA_REGISTRYINDEX, func_ref);
    // 引数をスタックに積む
    for (auto const& argument : { args... }) {
        lua_pushnumber(state_ptr, argument);
    }
    // 関数コール
    const int NUM_ARGS = static_cast<int>(sizeof...(args));
    const int NUM_RETVAL = 1;
    int ret = lua_pcall(state_ptr, NUM_ARGS, NUM_RETVAL, 0);
    // スタックから戻り値を得る
    RT retval = static_cast<RT>(0);
    if (ret == LUA_OK && lua_isnumber(state_ptr, LuaAccessorLocal::STACKTOP)) {
        retval = static_cast<RT>(lua_tonumber(state_ptr, LuaAccessorLocal::STACKTOP));
    }

    lua_pop(state_ptr, lua_gettop(state_ptr) - prev_stack_count);
    return retval;
}

} // namespace LuaAccessorLocal



/*!
 *  @brief
 */
LuaAccessor::LuaAccessor()
: m_pState(luaL_newstate())
, m_table_top_stack()
{
}
/*!
 *  @brief
 */
LuaAccessor::~LuaAccessor()
{
}
void LuaAccessor::luaStateDeleter::operator()(lua_State* state_ptr) const
{
    lua_close(state_ptr);
}

/*!
 *  @brief  ファイル読み込み
 */
bool LuaAccessor::DoFile(const std::string& file_name)
{
    return !luaL_dofile(m_pState.get(), file_name.c_str());
}

/*!
 *  @brief  スタックを全クリアする
 */
void LuaAccessor::ClearStack()
{  
    lua_State* state_ptr = m_pState.get();

    int now_top = lua_gettop(state_ptr);
    lua_pop(state_ptr, lua_gettop(state_ptr));
    int now_top2 = lua_gettop(state_ptr);
}


/*!
 *  @brief  親テーブルを開く
 *  @param  table_name  テーブル名
 *  @note   テーブルにアクセスできるようスタックに積んでおく(getglobal)
 */
int32_t LuaAccessor::OpenTable(const std::string& table_name)
{
    lua_State* state_ptr = m_pState.get();

    // 現在のスタック位置を保持
    m_table_top_stack.push(lua_gettop(state_ptr));
    //
    int32_t ret = lua_getglobal(state_ptr, table_name.c_str());
    if (ret == 0) {
        return -1;
    }
    else {
        return static_cast<int32_t>(lua_rawlen(state_ptr, LuaAccessorLocal::STACKTOP));
    }
}
/*!
 *  @brief  子テーブルを開く
 *  @param  table_name  子テーブル名
 *  @note   テーブル内テーブルにアクセスできるようスタックに積んでおく(getfield)
 */
int32_t LuaAccessor::OpenChildTable(const std::string& table_name)
{
    lua_State* state_ptr = m_pState.get();

    // 現在のスタック位置を保持
    m_table_top_stack.push(lua_gettop(state_ptr));
    //
    int32_t ret = lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, table_name.c_str());
    if (ret == 0) {
        return -1;
    }
    else {
        return static_cast<int32_t>(lua_rawlen(state_ptr, LuaAccessorLocal::STACKTOP));
    }
}
/*!
 *  @brief  子テーブルを開く
 *  @param  table_inx    子テーブル番号(0始まり)
 *  @note   テーブル内無名テーブルにアクセスできるようスタックに積んでおく(getfield)
 */
int32_t LuaAccessor::OpenChildTable(int32_t table_inx)
{
    lua_State* state_ptr = m_pState.get();

    // 現在のスタック位置を保持
    m_table_top_stack.push(lua_gettop(state_ptr));
    //
    int32_t lua_tbl_inx = table_inx + 1;
    int32_t ret = lua_geti(state_ptr, LuaAccessorLocal::STACKTOP, lua_tbl_inx);
    if (ret == 0) {
        return -1;
    }
    else {
        return static_cast<int32_t>(lua_rawlen(state_ptr, LuaAccessorLocal::STACKTOP));
    }
}
/*!
 *  @brief  最後に開いたテーブルをクローズする
 */
void LuaAccessor::CloseTable()
{
    lua_State* state_ptr = m_pState.get();

    // テーブルを開いて以降のスタック操作を綺麗にする
    lua_pop(state_ptr, lua_gettop(state_ptr) - m_table_top_stack.top());
    m_table_top_stack.pop();
}


/*!
 *  @brief  グローバルパラメータ取得
 *  @param[in]  param_name  パラメータ名
 *  @param[out] o_param     パラメータ格納先
 */
bool LuaAccessor::GetGlobalParam(const std::string& param_name, std::string& o_param)
{
    lua_State* state_ptr = m_pState.get();

    int32_t ret = lua_getglobal(state_ptr, param_name.c_str());
    if (ret == 0) {
        return false;
    }
    if (!lua_isstring(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = lua_tostring(state_ptr, LuaAccessorLocal::STACKTOP);
    return true;
}
bool LuaAccessor::GetGlobalParam(const std::string& param_name, int32_t& o_param)
{
    lua_State* state_ptr = m_pState.get();

    int32_t ret = lua_getglobal(state_ptr, param_name.c_str());
    if (ret == 0) {
        return false;
    }
    if (!lua_isinteger(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_param = static_cast<int32_t>(lua_tointeger(state_ptr, LuaAccessorLocal::STACKTOP));
    return true;
}

/*!
 *  @brief  テーブルパラメータ取得
 *  @param[in]  param_name  パラメータ名
 *  @param[out] o_param     パラメータ格納先
 */
bool LuaAccessor::GetTableParam(const std::string& param_name, std::string& o_param) { return LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_name, o_param); }
bool LuaAccessor::GetTableParam(const std::string& param_name, int32_t& o_param) { return  LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_name, o_param); }
bool LuaAccessor::GetTableParam(const std::string& param_name, float32& o_param) { return  LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_name, o_param); }
bool LuaAccessor::GetTableParam(const std::string& param_name, float64& o_param) { return  LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_name, o_param); }
bool LuaAccessor::GetTableParam(const std::string& param_name, bool& o_param) { return  LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_name, o_param); }
/*!
 *  @brief  配列パラメータ取得
 *  @param[in]  param_inx   パラメータインックス(0始まり)
 *  @param[out] o_param     パラメータ格納先
 */
bool LuaAccessor::GetArrayParam(int32_t param_inx, std::string& o_param) {return LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_inx, o_param); }
bool LuaAccessor::GetArrayParam(int32_t param_inx, int32_t& o_param) {return LuaAccessorLocal::GetTableParamWrapper(m_pState.get(), param_inx, o_param); }

/*!
*  @brief  lua関数リファレンス取得
*  @param[in]  param_name  パラメータ名(lua関数の格納されたパラメータの名前)
*  @param[out] o_ref       関数リファレンス格納先
*  @note   lua関数アドレスを取得・保持しておいてもそれをpushする仕組みが提供されていない
*  @note   関数アドレスを"不可侵管理領域"に登録し、必要なときに取り出して関数を呼び出すのが「正しい実装」とのこと
*  @note   ※関数アドレスはC側で保持していてもluaのGCを回避できない(無効化される)
*  @note   ※"不可侵領域"はluaが管理する領域なので参照が維持される、かつlua側からは触れない領域なので安全
*/
bool LuaAccessor::GetLuaFunctionReference(const std::string& param_name, int32_t& o_ref)
{
    lua_State* state_ptr = m_pState.get();

    int32_t ret = lua_getfield(state_ptr, LuaAccessorLocal::STACKTOP, param_name.c_str());
    if (ret == 0) {
        return false;
    }
    if (!lua_isfunction(state_ptr, LuaAccessorLocal::STACKTOP)) {
        return false;
    }
    o_ref = luaL_ref(state_ptr, LUA_REGISTRYINDEX);
    return true;
}
/*!
 *  @brief  lua関数呼び出し
 *  @param  func    関数ポインタ
 *  @param  fN      浮動小数点引数
 */
bool LuaAccessor::CallLuaBoolFunction(int32_t func_ref, float64 f1, float64 f2, float64 f3, float64 f4)
{
    return LuaAccessorLocal::CallLuaBoolFunction(m_pState.get(), func_ref, f1, f2, f3, f4);
}
float64 LuaAccessor::CallLuaFloatFunction(int32_t func_ref, float64 f1, float64 f2, float64 f3, float64 f4)
{
    return LuaAccessorLocal::CallLuaValueFunction<float64>(m_pState.get(), func_ref, f1, f2, f3, f4);
}

} // namespace
