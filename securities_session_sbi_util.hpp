/*!
 *  @file   securities_session_sbi_util.hpp
 *  @brief  SBI証券サイトとのセッション管理(utility)
 *  @date   2017/12/29
 *  @note   securities_session_sbiでしか使わないstaticな定数や関数群
 *  @note   ↑から直接includeする。ソース分割したいだけ。
 */


//! URL:SBI(PC):メインゲート
const wchar_t URL_MAIN_SBI_MAIN[] = L"https://site1.sbisec.co.jp/ETGate/";
//! URL:SBI(PC):ポートフォリオ転送トップ(site0へのログインを兼ねる)
const wchar_t URL_MAIN_SBI_TRANS_PF_LOGIN[] = L"?_ControlID=WPLETsmR001Control&_DataStoreID=DSWPLETsmR001Control&_PageID=WPLETsmR001Sdtl12&_ActionID=NoActionID&sw_page=WNS001&sw_param1=portfolio&sw_param2=pfcopy&cat1=home&cat2=none";
//! URL:SBI(PC):ポートフォリオ転送確認
const wchar_t URL_MAIN_SBI_TRANS_PF_CHECK[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/selectcheck.do";
//! URL:SBI(PC):ポートフォリオ転送実行
const wchar_t URL_MAIN_SBI_TRANS_PF_EXEC[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/transmission.do";
//! URL:SBI(PC):注文一覧
const wchar_t URL_MAIN_SBI_ORDER_LIST[] = L"?_ControlID=WPLETstT012Control&_PageID=DefaultPID&_DataStoreID=DSWPLETstT012Control&_ActionID=DefaultAID";
//! URL:SBI(mobile):ログイン
const wchar_t URL_BK_LOGIN[] = L"https://k.sbisec.co.jp/bsite/visitor/loginUserCheck.do";
//! URL:SBI(mobile):基本形
const wchar_t URL_BK_BASE[] = L"https://k.sbisec.co.jp/bsite/member/";
//! URL:SBI(mobile):トップページ
const wchar_t URL_BK_TOP[] = L"menu.do";
//! URL:SBI(mobile):余力
const wchar_t URL_BK_POSITIONMARGIN[] = L"acc/positionMargin.do";
//! URL:SBI(mobile):ポートフォリオ登録確認
const wchar_t URL_BK_STOCKENTRYCONFIRM[] = L"portfolio/lumpStockEntryConfirm.do";
//! URL:SBI(mobile):ポートフォリオ登録実行
const wchar_t URL_BK_STOCKENTRYEXECUTE[] = L"portfolio/lumpStockEntryExecute.do";
//! URL:SBI(mobile):注文訂正確認
const wchar_t URL_BK_CORRECTORDER_CONFIRM[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCorrectConfirm.do";
//! URL:SBI(mobile):注文訂正実行
const wchar_t URL_BK_CORRECTORDER_EXCUTE[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCorrectEx.do";
//! URL:SBI(mobile):注文取消実行
const wchar_t URL_BK_CANCELORDER_EXCUTE[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCancelEx.do";

//! パラメータ名
const wchar_t PARAM_NAME_PASSWORD[] = L"password";
//! パラメータ名：処理ユニークID
const wchar_t PARAM_NAME_REGIST_ID[] = L"regist_id";
//! パラメータ名：現物
const wchar_t PARAM_NAME_SPOT[] = L"stock/";
//! パラメータ名：信用
const wchar_t PARAM_NAME_LEVERAGE[] = L"margin/";
//! パラメータ名：売買注文：銘柄コード
const wchar_t PARAM_NAME_ORDER_STOCK_BRAND[] = L"brand_cd";
const wchar_t PARAM_NAME_ORDER_STOCK_CODE[] = L"ipm_product_code";
//! パラメータ名：売買注文：取引所種別
const wchar_t PARAM_NAME_ORDER_INVESTIMENTS[] = L"market";
//! パラメータ名：売買注文：価格
const wchar_t PARAM_NAME_ORDER_VALUE[] = L"price";
//! パラメータ名：売買注文：指値/成行き区分
const wchar_t PARAM_NAME_ORDER_MARKETORDER[] = L"sasinari_kbn";
//! パラメータ名：売買注文：信用区分
const wchar_t PARAM_NAME_LEVERAGE_CATEGORY[] = L"payment_limit";
//! パラメータ名：売買区分
const wchar_t PARAM_NAME_ORDER_TYPE[] = L"cayen.buysellKbn";
//! パラメータ名：注文訂正：注文番号(管理用)
const wchar_t PARAM_NAME_CORRECT_ORDER_ID[] = L"order_no";
//! パラメータ名：注文取消：注文番号(管理用)
const wchar_t PARAM_NAME_CANCEL_ORDER_ID[] = L"order_num";

//! ポートフォリオ番号：保有銘柄
const int32_t PORTFOLIO_ID_OWNED = 1;
//! ポートフォリオ番号：登録銘柄先頭
const int32_t PORTFOLIO_ID_USER_TOP = 2;

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
    { nullptr,                              nullptr,                        nullptr             },
    { L"buyOrderEntry.do",                  L"buyOrderEntryConfirm.do",     L"buyOrderEx.do"    },
    { L"sellOrderEntry.do",                 L"sellOrderEntryConfirm.do",    L"sellOrderEx.do"   },
    { L"orderCorrectEntry.do",              L"orderCorrectConfirm.do",      L"orderCorrectEx.do"},
    { L"orderCancelEntry.do",               nullptr,                        L"orderCancelEx.do" },
    { L"sellHOrderEntryTateListCheck.do",   L"sellHOrderEntryConfirm.do",   L"sellHOrderEx.do"  },
    { L"buyHOrderEntryTateListCheck.do",    L"buyHOrderEntryConfirm.do",    L"buyHOrderEx.do"   },
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
 *  @brief  ポートフォリオ表示URL構築
 *  @param  pf_id       ポートフォリオ番号
 *  @param  indicate_id 表示形式番号
 *  @return URL
 */
std::wstring BuildPortfolioURL(int32_t pf_id, int32_t indicate_id)
{
    std::wstring url(URL_MAIN_SBI_MAIN);
    utility_http::AddItemToURL(L"portforio_id", std::to_wstring(pf_id), url);
    utility_http::AddItemToURL(L"indicate_id", std::to_wstring(indicate_id), url);
    url += L"&sep_ind_specify_kbn=1&sort_kbn=0&sort_id=01&pts_kbn=0&_ControlID=WPLETpfR001Control&_PageID=WPLETpfR001Rlst10&_DataStoreID=DSWPLETpfR001Control&_ActionID=reloadPfList";
    return url;
}

/*!
 *  @brief  売買注文用URL構築
 *  @param  b_leverage  信用フラグ
 *  @param  type        注文種別
 *  @param  step        注文処理進行
 *  @return URL
 */
std::wstring BuildOrderURL(bool b_leverage, eOrderType type, eOrderStep step)
{
    const std::wstring sub_url(URL_BK_ORDER[type][step]);

    std::wstring url(std::move(std::wstring(URL_BK_BASE)));
    if (b_leverage) {
        url += PARAM_NAME_LEVERAGE + sub_url; // 信用
    } else {
        url += PARAM_NAME_SPOT + sub_url; // 現物
    }
    return url;
}

/*!
 *  @brief  返済売買建玉リスト取得URL構築
 *  @param  otype   注文種別
 *  @return URL
 */
std::wstring BuildRepOrderTateListURL(eOrderType odtype)
{
    if (odtype == ORDER_REPSELL) {
        return std::wstring(URL_BK_BASE) + PARAM_NAME_LEVERAGE + std::wstring(L"sellHOrderEntryTateList.do");
    } else {
        return std::wstring(URL_BK_BASE) + PARAM_NAME_LEVERAGE + std::wstring(L"buyHOrderEntryTateList.do");
    }
}

/*!
 *  @brief  注文制御用URL構築
 *  @param  sub_url     目的URL
 *  @return URL
 */
std::wstring BuildControlOrderURL(const std::wstring& sub_url)
{
    std::wstring url(std::move(std::wstring(URL_BK_BASE)));
    url += PARAM_NAME_SPOT + sub_url + L"?torihiki_kbn=1";
    return url;
}


/*!
 *  @brief  (mobileサイト)ログイン用FormData構築
 *  @param[in]  uid
 *  @param[in]  pwd     
 *  @param[out] request 格納先
 */
void BuildLoginFormData(const std::wstring& uid,
                        const std::wstring& pwd,
                        web::http::http_request& request)
{
    std::wstring form_data;
    utility_http::AddFormDataParamToString(L"username", uid, form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_PASSWORD, pwd, form_data);
    //
    utility_http::SetFormData(form_data, request);
}
/*!
 *  @brief  (PCサイト)ログイン用FormData構築
 *  @param[in]  uid
 *  @param[in]  pwd     
 *  @param[out] request 格納先
 */
void BuildLoginPCFormData(const std::wstring& uid,
                          const std::wstring& pwd,
                          web::http::http_request& request)
{
    const utility_http::sFormDataParam LOGIN_MAIN[] = {
        { L"JS_FLAG",           L"1" },
        { L"BW_FLG" ,           L"chrome,NaN" },
        { L"_ControlID",        L"WPLETlgR001Control" },
        { L"_DataStoreID",      L"DSWPLETlgR001Control" },
        { L"_PageID",           L"WPLETlgR001Rlgn10" },
        { L"_ActionID",         L"loginPortfolio" },
        { L"getFlg",            L"on" },
        { L"allPrmFlg",         L"" },
        { L"_ReturnPageInfo",   L"" },
    };
    std::wstring form_data;
    const size_t len = sizeof(LOGIN_MAIN)/sizeof(utility_http::sFormDataParam);
    for (uint32_t inx = 0; inx < len; inx++) {
        utility_http::AddFormDataParamToString(LOGIN_MAIN[inx], form_data);
    }
    utility_http::AddFormDataParamToString(L"user_id", uid, form_data);
    utility_http::AddFormDataParamToString(L"user_password", pwd, form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  ダミーの監視銘柄コード登録用FormData構築
 *  @param[in]  portfolio_number    登録先ポートフォリオ番号
 *  @param[in]  max_code_register   最大銘柄コード登録数
 *  @param[out] request             格納先
 *  @note   ※SBI(backup)の登録確認に投げるためのものなので中身はダミーで良い
 *  @note   ※予めSBI(backup)で対象となるポートフォリオを作成(枠確保)しておくこと
 */
void BuildDummyMonitoringCodeFormData(const int32_t portfolio_number,
                                      const size_t max_code_register,
                                      web::http::http_request& request)
{
    std::wstring form_data;

    // 'page'と'total_count'はダミーに限らず'0'固定
    utility_http::AddFormDataParamToString(L"page", L"0", form_data);
    utility_http::AddFormDataParamToString(L"list_number", std::to_wstring(portfolio_number), form_data);
    utility_http::AddFormDataParamToString(L"total_count", L"0", form_data);
    // 登録銘柄は空で良いがタグだけは必要
    const std::wstring NAME_NUMBER(L"npm");
    const std::wstring TAG_STOCK_CODE(L"pcode_");
    const std::wstring TAG_INVESTIMENTS_CODE(L"mcode_");
    const std::wstring blank_w(L"");
    for (size_t inx = 0; inx < max_code_register; inx++) {
        std::wstring index_str(std::to_wstring(inx));
        utility_http::AddFormDataParamToString(NAME_NUMBER, index_str, form_data);
        utility_http::AddFormDataParamToString(TAG_STOCK_CODE+index_str, blank_w, form_data);
        utility_http::AddFormDataParamToString(TAG_INVESTIMENTS_CODE+index_str, blank_w, form_data);
    }
    utility_http::AddFormDataParamToString(L"submit_update", L"登録・編集確認", form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  監視銘柄コード本登録用FormData構築
 *  @param[in]  portfolio_number    登録先ポートフォリオ番号
 *  @param[in]  max_code_register   最大銘柄コード登録数
 *  @param[in]  monitoring_code     監視銘柄コード
 *  @param[in]  investments_type    株取引所種別(全銘柄で共通)
 *  @param[in]  regist_id           処理ユニークID(登録確認応答から得る)
 *  @param[out] request             格納先
 */
void BuildMonitoringCodeFormData(const int32_t portfolio_number, 
                                 const size_t max_code_register,
                                 const StockCodeContainer& monitoring_code,
                                 eStockInvestmentsType investments_type,
                                 int64_t regist_id,
                                 web::http::http_request& request)
{
    std::wstring form_data;

    utility_http::AddFormDataParamToString(L"list_number", std::to_wstring(portfolio_number), form_data);
    //
    const std::wstring TAG_NUMBER(L"list_detail_number");
    const std::wstring TAG_STOCK_CODE(L"product_code");
    const std::wstring TAG_INVESTIMENTS_CODE(L"se_investments_code");
    const std::wstring investments_code(GetSbiInvestimentsCode(investments_type));
    size_t num_register = monitoring_code.size();
    {
        if (max_code_register < num_register) {
            num_register = max_code_register;
        }
        auto it = monitoring_code.begin();
        for (size_t inx = 0; inx < num_register && it != monitoring_code.end(); it++, inx++) {
            utility_http::AddFormDataParamToString(TAG_NUMBER, std::to_wstring(inx), form_data);
            utility_http::AddFormDataParamToString(TAG_STOCK_CODE, std::to_wstring(*it), form_data);
            utility_http::AddFormDataParamToString(TAG_INVESTIMENTS_CODE, investments_code, form_data);
        }
    }
    const std::wstring blank_w(L"");
    for (size_t inx = num_register; inx < max_code_register; inx++) {
        utility_http::AddFormDataParamToString(TAG_NUMBER, std::to_wstring(inx), form_data);
        utility_http::AddFormDataParamToString(TAG_STOCK_CODE, blank_w, form_data);
        utility_http::AddFormDataParamToString(TAG_INVESTIMENTS_CODE, blank_w, form_data);
    }
    utility_http::AddFormDataParamToString(L"submit", L"登録", form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  監視銘柄コード転送用FormData構築
 *  @param[out] request 格納先
 */
void BuildTransmitMonitoringCodeFormData(web::http::http_request& request)
{
    const utility_http::sFormDataParam PF_COPY[] = {
        { L"copyRequestNumber",         L""     },
        { L"sender_tool_from" ,         L"5"    },  // 5番はmobileサイト
        { L"receiver_tool_to",          L"1"    },  // 1番はPCサイト
        { L"select_add_replace_tool_01",L"1_2"  },  // '1_1'追加 '1_2'上書き
        { L"receivertoolListCnt",       L"4"    },  // 以下固定値っぽい
        { L"selectReceivertoolCnt",     L"1"    },
        { L"toolCode0",                 L"1"    },
        { L"disabled0",                 L"false"},
        { L"selected0",                 L"true" },
        { L"dispMsg0",                  L""     },
        { L"canNotAdd0",                L"false"},
        { L"count0",                    L""     },
        { L"radioName0",                L"select_add_replace_tool_01"   },
    };
    std::wstring form_data;
    const size_t len = sizeof(PF_COPY)/sizeof(utility_http::sFormDataParam);
    for (size_t inx = 0; inx < len; inx++) {
        utility_http::AddFormDataParamToString(PF_COPY[inx], form_data);
    }
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  注文用form data文字列に価格情報を入れる
 *  @param[in]  value       注文価格
 *  @param[in]  cond        注文条件
 *  @param[out] form_data   form data用文字列(格納先)
 */
void BuildOrderValueToFormDataString(float64 value, eOrderCondition cond, std::wstring& form_data)
{
    std::wstring motag;
    if (trade_utility::is_market_order(value)) {
        switch (cond)
        {
        case CONDITION_OPENING:
            motag = L"Y"; // 寄成
            break;
        case CONDITION_CLOSE:
            motag = L"H"; // 引成
            break;
        default:
            motag = L"N"; // 成行
            break;
        }
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_MARKETORDER, motag, form_data);
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_VALUE, L"", form_data);
    } else {
        switch (cond)
        {
        case CONDITION_OPENING:
            motag = L"Z"; // 寄指
            break;
        case CONDITION_CLOSE:
            motag = L"I"; // 引指
            break;
        case CONDITION_UNPROMOTED:
            motag = L"F"; // 不成
        default:
            break;
        }
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_MARKETORDER, motag, form_data);
        const int32_t VO = trade_utility::ValueOrder();
        const std::wstring value_str(std::move(utility_string::ToWstringOrder(value, VO)));
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_VALUE, value_str, form_data);
    }
}

/*!
 *  @brief  新規注文用FormData構築
 *  @param[in]  order       売買注文
 *  @param[in]  pass
 *  @param[in]  regist_id   処理ユニークID(前処理から得る)
 *  @param[out] request     格納先
 */
void BuildFreshOrderFormData(const StockOrder& order, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;

    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.isStopOrder",     L"false"},  // 逆差しフラグ(=通常) 逆差しならtrue
        { L"caLiKbn" ,              L"today"},  // 期間(=当日中)
        { L"limit",                 L""     },  // 期間指定(=当日中) 当日中以外はYYYYMMDDで指定
        { L"hitokutei_trade_kbn",   L"-"    },  // 預かり区分(一般) 特定口座なら変わるっぽい
        { L"cayen.sor_select",      L"false"},  // SOR不要
    };
    for (uint32_t inx = 0; inx < sizeof(ORDER_FORM)/sizeof(utility_http::sFormDataParam); inx++) {
        utility_http::AddFormDataParamToString(ORDER_FORM[inx], form_data);
    }
    utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_STOCK_BRAND, std::to_wstring(order.GetCode()), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.GetCode()), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_INVESTIMENTS, GetSbiInvestimentsCode(order.m_investments), form_data);
    utility_http::AddFormDataParamToString(L"quantity", std::to_wstring(order.m_number), form_data);
    BuildOrderValueToFormDataString(order.m_value, order.m_condition, form_data);
    //
    if (!pass.empty()) {
        utility_http::AddFormDataParamToString(PARAM_NAME_PASSWORD, pass, form_data);
    }
    if (order.m_b_leverage) {
        utility_http::AddFormDataParamToString(PARAM_NAME_LEVERAGE_CATEGORY, L"6", form_data);   // >ToDo< 一般信用/日計りの対応
    }
    switch (order.m_type)
    {
    case ORDER_BUY:
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_TYPE, L"buy", form_data);
        break;
    case ORDER_SELL:
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_TYPE, L"sell", form_data);
        break;
    }
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  信用返済注文用FormData構築
 *  @param[in]  caIQ
 *  @param[in]  quantity_tag
 *  @param[in]  order           売買注文
 *  @param[in]  pass
 *  @param[in]  regist_id       処理ユニークID(前処理から得る)
 *  @param[out] request         格納先
 */
void BuildRepaymenLeverageOrderFormdata(const std::string& caIQ,
                                        const std::string& quantity_tag,
                                        const StockOrder& order,
                                        const std::wstring& pass,
                                        int64_t regist_id,
                                        web::http::http_request& request)
{
    std::wstring form_data;
    std::wstring caIQ_w(std::move(utility_string::ToWstring(caIQ)));

    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"open_trade_kbn",    L"1"    },  // ???
        { L"caIsLump",          L"false"},  // 返済建玉指定方法(=個別) 一括ならtrue
        { L"request_type",      L"16"   },  // 返済順序(=建日古い順)
        { L"caLiKbn" ,          L"today"},  // 期間(=当日中)
        { L"limit",             L""     },  // 期間指定(=当日中)
        { L"cayen.isStopOrder", L"false"},  // 逆差しフラグ(=通常) 逆差しならtrue
    };
    for (uint32_t inx = 0; inx < sizeof(ORDER_FORM)/sizeof(utility_http::sFormDataParam); inx++) {
        utility_http::AddFormDataParamToString(ORDER_FORM[inx], form_data);
    }
    if (regist_id > 0) {
        utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    }
    utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_STOCK_BRAND, std::to_wstring(order.m_code.GetCode()), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_STOCK_CODE, std::to_wstring(order.m_code.GetCode()), form_data);
    utility_http::AddFormDataParamToString(L"open_market", GetSbiInvestimentsCode(order.m_investments), form_data);
    utility_http::AddFormDataParamToString(L"sum_quantity", std::to_wstring(order.m_number), form_data);
    BuildOrderValueToFormDataString(order.m_value, order.m_condition, form_data);
    //
    utility_http::AddFormDataParamToString(PARAM_NAME_LEVERAGE_CATEGORY, L"6", form_data);   // >ToDo< 一般信用/日計りの対応
    //
    if (!quantity_tag.empty()) {
        std::wstring quantity_tag_w(std::move(utility_string::ToWstring(quantity_tag)));
        utility_http::AddFormDataParamToString(quantity_tag_w, std::to_wstring(order.m_number), form_data);
        utility_http::AddFormDataParamToString(L"caIQ", caIQ_w, form_data);
        utility_http::AddFormDataParamToString(L"caPSO", L"61", form_data);
        utility_http::AddFormDataParamToString(L"decision", L"建玉確定", form_data);
    } else {
        auto pos = caIQ_w.find(L":,");
        if (pos > 0) {
            caIQ_w.replace(pos, caIQ_w.length(), std::to_wstring(order.m_number));
            utility_http::AddFormDataParamToString(L"caIQ", caIQ_w, form_data);
        }
    }
    if (!pass.empty()) {
        utility_http::AddFormDataParamToString(PARAM_NAME_PASSWORD, pass, form_data);
    }
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  注文訂正用FormData構築
 *  @param[in]  order_id    注文番号(管理用)
 *  @param[in]  order       訂正注文
 *  @param[in]  pass
 *  @param[in]  regist_id   処理ユニークID(前処理から得る)
 *  @param[out] request     格納先
 */
void BuildCorrectOrderFormData(int32_t order_id, const StockOrder& order, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;

    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.quryOrderStatus", L"order"},  // ???
        { L"torihiki_kbn" ,         L"1"    },  // 取引区分(とは?)
    };
    for (uint32_t inx = 0; inx < sizeof(ORDER_FORM)/sizeof(utility_http::sFormDataParam); inx++) {
        utility_http::AddFormDataParamToString(ORDER_FORM[inx], form_data);
    }
    utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_CORRECT_ORDER_ID, std::to_wstring(order_id), form_data);
    if (!pass.empty()) {
        utility_http::AddFormDataParamToString(PARAM_NAME_PASSWORD, pass, form_data);
    }
    BuildOrderValueToFormDataString(order.m_value, order.m_condition, form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  注文取消し用FormData構築
 *  @param[in]  order_id    注文番号(管理用)
 *  @param[in]  pass
 *  @param[in]  regist_id   処理ユニークID(前処理から得る)
 *  @param[out] request     格納先
 */
void BuildCancelOrderFormData(int32_t order_id, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;
    //
    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.quryOrderStatus", L"order"},  // ???
        { L"torihiki_kbn" ,         L"1"    },  // 取引区分(とは?)
    };
    for (uint32_t inx = 0; inx < sizeof(ORDER_FORM)/sizeof(utility_http::sFormDataParam); inx++) {
        utility_http::AddFormDataParamToString(ORDER_FORM[inx], form_data);
    }
    utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_CANCEL_ORDER_ID, std::to_wstring(order_id), form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_PASSWORD, pass, form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  responseStockOrderExecの結果をRcvResponseStockOrderへ格納
 *  @param[in]  t           スクリプト結果tuple
 *  @param[out] rcv_order   格納先
 */
bool ToRcvResponseStockOrderFrom_responseStockOrderExec(const boost::python::tuple& t,
                                                        RcvResponseStockOrder& rcv_order)
{
    using boost::python::extract;

    const bool b_result = extract<bool>(t[0]);
    rcv_order.m_order_id = extract<int32_t>(t[1]);
    rcv_order.m_user_order_id = extract<int32_t>(t[2]);
    rcv_order.m_investments = GetStockInvestmentsTypeFromSbiCode(extract<std::string>(t[3]));
    rcv_order.m_code = extract<uint32_t>(t[4]);
    rcv_order.m_number = extract<uint32_t>(t[5]);
    rcv_order.m_value = extract<float64>(t[6]);
    rcv_order.m_b_leverage = extract<bool>(t[7]);
    const int32_t order_type = extract<int32_t>(t[8]);
    rcv_order.m_type = static_cast<eOrderType>(order_type);

    return b_result;
}
