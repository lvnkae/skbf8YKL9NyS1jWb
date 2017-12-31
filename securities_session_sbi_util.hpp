/*!
 *  @file   securities_session_sbi_util.hpp
 *  @brief  SBI証券サイトとのセッション管理(utility)
 *  @date   2017/12/29
 *  @note   securities_session_sbiでしか使わないstaticな定数や関数群
 *  @note   ↑から直接includeする。ソース分割したいだけ。
 */


//! URL:SBI(PC):メインゲート
const wchar_t URL_MAIN_SBI_MAIN[] = L"https://site1.sbisec.co.jp/ETGate/";
//! URL:SBI(PC):ポートフォリオ表示
const wchar_t URL_MAIN_SBI_PORTFOLIO[] = L"?portforio_id=2&indicate_id=5&sep_ind_specify_kbn=0&sort_kbn=0&sort_id=01&pts_kbn=0&_ControlID=WPLETpfR001Control&_PageID=WPLETpfR001Rlst10&_DataStoreID=DSWPLETpfR001Control&_ActionID=reloadPfList";
//! URL:SBI(PC):ポートフォリオ転送トップ(site0へのログインを兼ねる)
const wchar_t URL_MAIN_SBI_TRANS_PF_LOGIN[] = L"?_ControlID=WPLETsmR001Control&_DataStoreID=DSWPLETsmR001Control&_PageID=WPLETsmR001Sdtl12&_ActionID=NoActionID&sw_page=WNS001&sw_param1=portfolio&sw_param2=pfcopy&cat1=home&cat2=none";
//! URL:SBI(PC):ポートフォリオ転送確認
const wchar_t URL_MAIN_SBI_TRANS_PF_CHECK[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/selectcheck.do";
//! URL:SBI(PC):ポートフォリオ転送実行
const wchar_t URL_MAIN_SBI_TRANS_PF_EXEC[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/transmission.do";
//! URL:SBI(mobile):ログイン
const wchar_t URL_BK_LOGIN[] = L"https://k.sbisec.co.jp/bsite/visitor/loginUserCheck.do";
//! URL:SBI(mobile):基本形
const wchar_t URL_BK_BASE[] = L"https://k.sbisec.co.jp/bsite/member/";
//! URL:SBI(mobile):トップページ
const wchar_t URL_BK_TOP[] = L"menu.do";
//! URL:SBI(mobile):ポートフォリオ登録確認
const wchar_t URL_BK_STOCKENTRYCONFIRM[] = L"portfolio/lumpStockEntryConfirm.do";
//! URL:SBI(mobile):ポートフォリオ登録実行
const wchar_t URL_BK_STOCKENTRYEXECUTE[] = L"portfolio/lumpStockEntryExecute.do";
//! URL:SBI(mobile):注文入力(買)
const wchar_t URL_BK_BUYORDER_ENTRY[] = L"buyOrderEntry.do";
//! URL:SBI(mobile):注文確認(買)
const wchar_t URL_BK_BUYORDER_CONFIRM[] = L"buyOrderEntryConfirm.do";
//! URL:SBI(mobile):注文発行(買)
const wchar_t URL_BK_BUYORDER_EXECUTE[] = L"buyOrderEx.do";
//! URL:SBI(mobile):注文入力(買)
const wchar_t URL_BK_SELLORDER_ENTRY[] = L"sellOrderEntry.do";
//! URL:SBI(mobile):注文確認(買)
const wchar_t URL_BK_SELLORDER_CONFIRM[] = L"sellOrderEntryConfirm.do";
//! URL:SBI(mobile):注文発行(売)
const wchar_t URL_BK_SELLORDER_EXECUTE[] = L"sellOrderEx.do";

//! パラメータ名：現物
const wchar_t PARAM_NAME_SPOT[] = L"stock/";
//! パラメータ名：信用
const wchar_t PARAM_NAME_LEVERAGE[] = L"margin/";
//! パラメータ名：注文時銘柄コード
const wchar_t PARAM_NAME_ORDER_STOCK_CODE[] = L"ipm_product_code";
//! パラメータ名：注文時取引所種別
const wchar_t PARAM_NAME_ORDER_INVESTIMENTS[] = L"market";
//! パラメータ名：売買区分
const wchar_t PARAM_NAME_ORDER_TYPE[] = L"cayen.buysellKbn";

/*!
 *  @brief  注文ステップ
 */
enum eOrderStep {
    OSTEP_INPUT,    // 入力
    OSTEP_CONFIRM,  // 確認
    OSTEP_EXECUTE,  // 実行

    NUM_OSTEP,
};
const wchar_t* URL_BK_ORDER[NUM_ORDER][NUM_OSTEP] = {
    { nullptr,                  nullptr,                    nullptr                     },
    { URL_BK_BUYORDER_ENTRY,    URL_BK_BUYORDER_CONFIRM,    URL_BK_BUYORDER_EXECUTE     },
    { URL_BK_SELLORDER_ENTRY,   URL_BK_SELLORDER_CONFIRM,   URL_BK_SELLORDER_EXECUTE    },
    { nullptr,                  nullptr,                    nullptr                     },
    { nullptr,                  nullptr,                    nullptr                     },
};

/*!
 *  @brief  取引所種別を"SBI用銘柄登録用取引所コード"に変換
 *  @param  investments_type    取引所種別
 */
std::wstring GetSbiInvestimentsCode(eStockInvestmentsType investments_type)
{
    switch (investments_type)
    {
    case INVESTMENTS_TOKYO:
        return L"TKY";
    case INVESTMENTS_NAGOYA:
        return L"NGY";
    case INVESTMENTS_FUKUOKA:
        return L"FKO";
    case INVESTMENTS_SAPPORO:
        return L"SPR";
    case INVESTMENTS_PTS:
        return L"JNX";
    default:
        return L"";
    }
}

/*!
 *  @brief  "SBI用銘柄登録用取引所コード"を取引所種別をに変換
 *  @param  code    SBI用銘柄登録用取引所コード
 */
eStockInvestmentsType GetStockInvestmentsTypeFromSbiCode(const std::string& code)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utfconv;
    std::wstring codeT = utfconv.from_bytes(code.c_str());

    if (GetSbiInvestimentsCode(INVESTMENTS_TOKYO).compare(codeT) == 0) {
        return INVESTMENTS_TOKYO;
    }
    if (GetSbiInvestimentsCode(INVESTMENTS_PTS).compare(codeT) == 0) {
        return INVESTMENTS_PTS;
    }
    if (GetSbiInvestimentsCode(INVESTMENTS_NAGOYA).compare(codeT) == 0) {
        return INVESTMENTS_NAGOYA;
    }
    if (GetSbiInvestimentsCode(INVESTMENTS_FUKUOKA).compare(codeT) == 0) {
        return INVESTMENTS_FUKUOKA;
    }
    if (GetSbiInvestimentsCode(INVESTMENTS_SAPPORO).compare(codeT) == 0) {
        return INVESTMENTS_SAPPORO;
    }
    return INVESTMENTS_NONE;
}

/*!
 *  @brief  売買注文用URL構築
 *  @param[in]  b_leverage  信用フラグ
 *  @param[in]  sub_url     目的URL
 *  @param[out] url         格納先
 */
void BuildOrderURL(bool b_leverage, const std::wstring& sub_url, std::wstring& url)
{
    url = URL_BK_BASE;
    if (b_leverage) {
        url += PARAM_NAME_LEVERAGE + sub_url; // 信用
    } else {
        url += PARAM_NAME_SPOT + sub_url; // 現物
    }
}

/*!
 *  @brief  ダミーのポートフォリオ文字列(form data用)構築
 *  @param[in]  portfolio_number    登録先ポートフォリオ番号
 *  @param[in]  max_code_entry      最大銘柄登録数
 *  @param[out] dst                 格納先
 *  @note   ※SBI(backup)の登録確認に投げるためのものなので中身はダミーで良い
 *  @note   ※予めSBU(backup)で対象となるポートフォリオを作成(枠確保)しておくこと
 */
void BuildDummyPortfolioForFormData(const int32_t portfolio_number, const size_t max_code_entry, std::wstring& dst)
{
    // 'page'と'total_count'はダミーに限らず'0'固定
    utility::AddFormDataParamToString(L"page", L"0", dst);
    utility::AddFormDataParamToString(L"list_number", std::to_wstring(portfolio_number), dst);
    utility::AddFormDataParamToString(L"total_count", L"0", dst);
    // 登録銘柄は空で良いがタグだけは必要
    const std::wstring NAME_NUMBER(L"npm");
    const std::wstring TAG_STOCK_CODE(L"pcode_");
    const std::wstring TAG_INVESTIMENTS_CODE(L"mcode_");
    for (size_t inx = 0; inx < max_code_entry; inx++) {
        std::wstring index_str(std::to_wstring(inx));
        utility::AddFormDataParamToString(NAME_NUMBER, index_str, dst);
        utility::AddFormDataParamToString(TAG_STOCK_CODE+index_str, L"", dst);
        utility::AddFormDataParamToString(TAG_INVESTIMENTS_CODE+index_str, L"", dst);
    }
    utility::AddFormDataParamToString(L"submit_update", L"登録・編集確認", dst);
}

/*!
 *  @brief  本登録用ポートフォリオ文字列(form data用)構築
 *  @param[in]  portfolio_number    登録先ポートフォリオ番号
 *  @param[in]  max_code_entry      最大銘柄登録数
 *  @param[in]  monitoring_code     登録(監視)銘柄
 *  @param[in]  investments_type    株取引助手別(全銘柄で共通)
 *  @param[in]  regist_id           登録用ユニークID(登録確認応答から得る)
 *  @param[out] dst                 格納先
 */
void BuildPortfolioForFormData(const int32_t portfolio_number, 
                               const size_t max_code_entry,
                               const std::vector<uint32_t>& monitoring_code,
                               eStockInvestmentsType investments_type,
                               int64_t regist_id,
                               std::wstring& dst)
{
    utility::AddFormDataParamToString(L"list_number", std::to_wstring(portfolio_number), dst);
    //
    const std::wstring TAG_NUMBER(L"list_detail_number");
    const std::wstring TAG_STOCK_CODE(L"product_code");
    const std::wstring TAG_INVESTIMENTS_CODE(L"se_investments_code");
    const std::wstring investments_code(GetSbiInvestimentsCode(investments_type));
    size_t entry = monitoring_code.size();
    if (max_code_entry < entry) {
        entry = max_code_entry;
    }
    for (size_t inx = 0; inx < entry; inx++) {
        utility::AddFormDataParamToString(TAG_NUMBER, std::to_wstring(inx), dst);
        utility::AddFormDataParamToString(TAG_STOCK_CODE, std::to_wstring(monitoring_code[inx]), dst);
        utility::AddFormDataParamToString(TAG_INVESTIMENTS_CODE, investments_code, dst);
    }
    for (size_t inx = entry; inx < max_code_entry; inx++) {
        utility::AddFormDataParamToString(TAG_NUMBER, std::to_wstring(inx), dst);
        utility::AddFormDataParamToString(TAG_STOCK_CODE, L"", dst);
        utility::AddFormDataParamToString(TAG_INVESTIMENTS_CODE, L"", dst);
    }
    utility::AddFormDataParamToString(L"submit", L"登録", dst);
    utility::AddFormDataParamToString(L"regist_id", std::to_wstring(regist_id), dst);
}

/*!
 *  @brief  売買注文用文字列(form data用)構築
 *  @param[in]  order       売買命令
 *  @param[in]  pass
 *  @param[in]  regist_id   登録用ユニークID(前処理から得る)
 *  @param[out] dst         格納先
 */
void BuildOrderForFormData(const StockOrder& order, const std::wstring& pass, int64_t regist_id, std::wstring& dst)
{
    const utility::sFormDataParam ORDER_FORM[] = {
        { L"cayen.isStopOrder",         L"false"},  // 逆差しフラグ
        { L"caLiKbn" ,                  L"today"},  // 期間
        { L"limit",                     L""     },  // 期間指定(当日中以外はYYYYMMDDで指定)
        { L"hitokutei_trade_kbn",       L"-"    },  // 預かり区分(一般) 特定口座なら変わるっぽい
        { L"cayen.sor_select",          L"false"},  // SOR不要
    };
    for (uint32_t inx = 0; inx < sizeof(ORDER_FORM)/sizeof(utility::sFormDataParam); inx++) {
        utility::AddFormDataParamToString(ORDER_FORM[inx], dst);
    }
    utility::AddFormDataParamToString(L"regist_id", std::to_wstring(regist_id), dst);
    utility::AddFormDataParamToString(L"brand_cd", std::to_wstring(order.m_code.GetCode()), dst);
    utility::AddFormDataParamToString(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), dst);
    utility::AddFormDataParamToString(L"market", GetSbiInvestimentsCode(order.m_investiments), dst);
    utility::AddFormDataParamToString(L"quantity", std::to_wstring(order.m_number), dst);
    utility::AddFormDataParamToString(L"price", utility::ToWstringOrder(order.m_value, 1), dst); // 小数点第1位まで採用
    utility::AddFormDataParamToString(L"password", pass, dst);
    if (order.m_b_leverage) {
        utility::AddFormDataParamToString(L"payment_limit", L"6", dst);   // >ToDo< 一般信用/日計りの対応
    }
    switch (order.m_type)
    {
    case ORDER_BUY:
        utility::AddFormDataParamToString(PARAM_NAME_ORDER_TYPE, L"buy", dst);
        break;
    case ORDER_SELL:
        utility::AddFormDataParamToString(PARAM_NAME_ORDER_TYPE, L"sell", dst);
        break;
    };
}
