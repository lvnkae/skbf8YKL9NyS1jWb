/*!
 *  @file   securities_session_sbi_util.hpp
 *  @brief  SBI�،��T�C�g�Ƃ̃Z�b�V�����Ǘ�(utility)
 *  @date   2017/12/29
 *  @note   securities_session_sbi�ł����g��Ȃ�static�Ȓ萔��֐��Q
 *  @note   �����璼��include����B�\�[�X���������������B
 */


//! URL:SBI(PC):���C���Q�[�g
const wchar_t URL_MAIN_SBI_MAIN[] = L"https://site1.sbisec.co.jp/ETGate/";
//! URL:SBI(PC):�|�[�g�t�H���I�]���g�b�v(site0�ւ̃��O�C�������˂�)
const wchar_t URL_MAIN_SBI_TRANS_PF_LOGIN[] = L"?_ControlID=WPLETsmR001Control&_DataStoreID=DSWPLETsmR001Control&_PageID=WPLETsmR001Sdtl12&_ActionID=NoActionID&sw_page=WNS001&sw_param1=portfolio&sw_param2=pfcopy&cat1=home&cat2=none";
//! URL:SBI(PC):�|�[�g�t�H���I�]���m�F
const wchar_t URL_MAIN_SBI_TRANS_PF_CHECK[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/selectcheck.do";
//! URL:SBI(PC):�|�[�g�t�H���I�]�����s
const wchar_t URL_MAIN_SBI_TRANS_PF_EXEC[] = L"https://site0.sbisec.co.jp/marble/portfolio/pfcopy/transmission.do";
//! URL:SBI(PC):�����ꗗ
const wchar_t URL_MAIN_SBI_ORDER_LIST[] = L"?_ControlID=WPLETstT012Control&_PageID=DefaultPID&_DataStoreID=DSWPLETstT012Control&_ActionID=DefaultAID";
//! URL:SBI(mobile):���O�C��
const wchar_t URL_BK_LOGIN[] = L"https://k.sbisec.co.jp/bsite/visitor/loginUserCheck.do";
//! URL:SBI(mobile):��{�`
const wchar_t URL_BK_BASE[] = L"https://k.sbisec.co.jp/bsite/member/";
//! URL:SBI(mobile):�g�b�v�y�[�W
const wchar_t URL_BK_TOP[] = L"menu.do";
//! URL:SBI(mobile):�]��
const wchar_t URL_BK_POSITIONMARGIN[] = L"acc/positionMargin.do";
//! URL:SBI(mobile):�|�[�g�t�H���I�o�^�m�F
const wchar_t URL_BK_STOCKENTRYCONFIRM[] = L"portfolio/lumpStockEntryConfirm.do";
//! URL:SBI(mobile):�|�[�g�t�H���I�o�^���s
const wchar_t URL_BK_STOCKENTRYEXECUTE[] = L"portfolio/lumpStockEntryExecute.do";
//! URL:SBI(mobile):���������m�F
const wchar_t URL_BK_CORRECTORDER_CONFIRM[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCorrectConfirm.do";
//! URL:SBI(mobile):�����������s
const wchar_t URL_BK_CORRECTORDER_EXCUTE[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCorrectEx.do";
//! URL:SBI(mobile):����������s
const wchar_t URL_BK_CANCELORDER_EXCUTE[] = L"https://k.sbisec.co.jp/bsite/member/stock/orderCancelEx.do";

//! �p�����[�^��
const wchar_t PARAM_NAME_PASSWORD[] = L"password";
//! �p�����[�^���F�������j�[�NID
const wchar_t PARAM_NAME_REGIST_ID[] = L"regist_id";
//! �p�����[�^���F����
const wchar_t PARAM_NAME_SPOT[] = L"stock/";
//! �p�����[�^���F�M�p
const wchar_t PARAM_NAME_LEVERAGE[] = L"margin/";
//! �p�����[�^���F���������F�����R�[�h
const wchar_t PARAM_NAME_ORDER_STOCK_BRAND[] = L"brand_cd";
const wchar_t PARAM_NAME_ORDER_STOCK_CODE[] = L"ipm_product_code";
//! �p�����[�^���F���������F��������
const wchar_t PARAM_NAME_ORDER_INVESTIMENTS[] = L"market";
//! �p�����[�^���F���������F���i
const wchar_t PARAM_NAME_ORDER_VALUE[] = L"price";
//! �p�����[�^���F���������F�w�l/���s���敪
const wchar_t PARAM_NAME_ORDER_MARKETORDER[] = L"sasinari_kbn";
//! �p�����[�^���F���������F�M�p�敪
const wchar_t PARAM_NAME_LEVERAGE_CATEGORY[] = L"payment_limit";
//! �p�����[�^���F�����敪
const wchar_t PARAM_NAME_ORDER_TYPE[] = L"cayen.buysellKbn";
//! �p�����[�^���F���������F�����ԍ�(�Ǘ��p)
const wchar_t PARAM_NAME_CORRECT_ORDER_ID[] = L"order_no";
//! �p�����[�^���F��������F�����ԍ�(�Ǘ��p)
const wchar_t PARAM_NAME_CANCEL_ORDER_ID[] = L"order_num";

//! �|�[�g�t�H���I�ԍ��F�ۗL����
const int32_t PORTFOLIO_ID_OWNED = 1;
//! �|�[�g�t�H���I�ԍ��F�o�^�����擪
const int32_t PORTFOLIO_ID_USER_TOP = 2;

/*!
 *  @brief  �����X�e�b�v
 */
enum eOrderStep {
    OSTEP_INPUT,    // ����
    OSTEP_CONFIRM,  // �m�F
    OSTEP_EXECUTE,  // ���s

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
 *  @brief  �������ʂ�"SBI�p�����o�^�p������R�[�h"�ɕϊ�
 *  @param  investments_type    ��������
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
 *  @brief  "SBI�p�����o�^�p������R�[�h"���������ʂ��ɕϊ�
 *  @param  code    SBI�p�����o�^�p������R�[�h
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
 *  @brief  �|�[�g�t�H���I�\��URL�\�z
 *  @param  pf_id       �|�[�g�t�H���I�ԍ�
 *  @param  indicate_id �\���`���ԍ�
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
 *  @brief  ���������pURL�\�z
 *  @param  b_leverage  �M�p�t���O
 *  @param  type        �������
 *  @param  step        ���������i�s
 *  @return URL
 */
std::wstring BuildOrderURL(bool b_leverage, eOrderType type, eOrderStep step)
{
    const std::wstring sub_url(URL_BK_ORDER[type][step]);

    std::wstring url(std::move(std::wstring(URL_BK_BASE)));
    if (b_leverage) {
        url += PARAM_NAME_LEVERAGE + sub_url; // �M�p
    } else {
        url += PARAM_NAME_SPOT + sub_url; // ����
    }
    return url;
}

/*!
 *  @brief  �ԍϔ������ʃ��X�g�擾URL�\�z
 *  @param  otype   �������
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
 *  @brief  ��������pURL�\�z
 *  @param  sub_url     �ړIURL
 *  @return URL
 */
std::wstring BuildControlOrderURL(const std::wstring& sub_url)
{
    std::wstring url(std::move(std::wstring(URL_BK_BASE)));
    url += PARAM_NAME_SPOT + sub_url + L"?torihiki_kbn=1";
    return url;
}


/*!
 *  @brief  (mobile�T�C�g)���O�C���pFormData�\�z
 *  @param[in]  uid
 *  @param[in]  pwd     
 *  @param[out] request �i�[��
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
 *  @brief  (PC�T�C�g)���O�C���pFormData�\�z
 *  @param[in]  uid
 *  @param[in]  pwd     
 *  @param[out] request �i�[��
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
 *  @brief  �_�~�[�̊Ď������R�[�h�o�^�pFormData�\�z
 *  @param[in]  portfolio_number    �o�^��|�[�g�t�H���I�ԍ�
 *  @param[in]  max_code_register   �ő�����R�[�h�o�^��
 *  @param[out] request             �i�[��
 *  @note   ��SBI(backup)�̓o�^�m�F�ɓ����邽�߂̂��̂Ȃ̂Œ��g�̓_�~�[�ŗǂ�
 *  @note   ���\��SBI(backup)�őΏۂƂȂ�|�[�g�t�H���I���쐬(�g�m��)���Ă�������
 */
void BuildDummyMonitoringCodeFormData(const int32_t portfolio_number,
                                      const size_t max_code_register,
                                      web::http::http_request& request)
{
    std::wstring form_data;

    // 'page'��'total_count'�̓_�~�[�Ɍ��炸'0'�Œ�
    utility_http::AddFormDataParamToString(L"page", L"0", form_data);
    utility_http::AddFormDataParamToString(L"list_number", std::to_wstring(portfolio_number), form_data);
    utility_http::AddFormDataParamToString(L"total_count", L"0", form_data);
    // �o�^�����͋�ŗǂ����^�O�����͕K�v
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
    utility_http::AddFormDataParamToString(L"submit_update", L"�o�^�E�ҏW�m�F", form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  �Ď������R�[�h�{�o�^�pFormData�\�z
 *  @param[in]  portfolio_number    �o�^��|�[�g�t�H���I�ԍ�
 *  @param[in]  max_code_register   �ő�����R�[�h�o�^��
 *  @param[in]  monitoring_code     �Ď������R�[�h
 *  @param[in]  investments_type    ����������(�S�����ŋ���)
 *  @param[in]  regist_id           �������j�[�NID(�o�^�m�F�������瓾��)
 *  @param[out] request             �i�[��
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
    utility_http::AddFormDataParamToString(L"submit", L"�o�^", form_data);
    utility_http::AddFormDataParamToString(PARAM_NAME_REGIST_ID, std::to_wstring(regist_id), form_data);
    //
    utility_http::SetFormData(form_data, request);
}

/*!
 *  @brief  �Ď������R�[�h�]���pFormData�\�z
 *  @param[out] request �i�[��
 */
void BuildTransmitMonitoringCodeFormData(web::http::http_request& request)
{
    const utility_http::sFormDataParam PF_COPY[] = {
        { L"copyRequestNumber",         L""     },
        { L"sender_tool_from" ,         L"5"    },  // 5�Ԃ�mobile�T�C�g
        { L"receiver_tool_to",          L"1"    },  // 1�Ԃ�PC�T�C�g
        { L"select_add_replace_tool_01",L"1_2"  },  // '1_1'�ǉ� '1_2'�㏑��
        { L"receivertoolListCnt",       L"4"    },  // �ȉ��Œ�l���ۂ�
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
 *  @brief  �����pform data������ɉ��i��������
 *  @param[in]  value       �������i
 *  @param[in]  cond        ��������
 *  @param[out] form_data   form data�p������(�i�[��)
 */
void BuildOrderValueToFormDataString(float64 value, eOrderCondition cond, std::wstring& form_data)
{
    std::wstring motag;
    if (trade_utility::is_market_order(value)) {
        switch (cond)
        {
        case CONDITION_OPENING:
            motag = L"Y"; // ��
            break;
        case CONDITION_CLOSE:
            motag = L"H"; // ����
            break;
        default:
            motag = L"N"; // ���s
            break;
        }
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_MARKETORDER, motag, form_data);
        utility_http::AddFormDataParamToString(PARAM_NAME_ORDER_VALUE, L"", form_data);
    } else {
        switch (cond)
        {
        case CONDITION_OPENING:
            motag = L"Z"; // ��w
            break;
        case CONDITION_CLOSE:
            motag = L"I"; // ���w
            break;
        case CONDITION_UNPROMOTED:
            motag = L"F"; // �s��
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
 *  @brief  �V�K�����pFormData�\�z
 *  @param[in]  order       ��������
 *  @param[in]  pass
 *  @param[in]  regist_id   �������j�[�NID(�O�������瓾��)
 *  @param[out] request     �i�[��
 */
void BuildFreshOrderFormData(const StockOrder& order, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;

    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.isStopOrder",     L"false"},  // �t�����t���O(=�ʏ�) �t�����Ȃ�true
        { L"caLiKbn" ,              L"today"},  // ����(=������)
        { L"limit",                 L""     },  // ���Ԏw��(=������) �������ȊO��YYYYMMDD�Ŏw��
        { L"hitokutei_trade_kbn",   L"-"    },  // �a����敪(���) ��������Ȃ�ς����ۂ�
        { L"cayen.sor_select",      L"false"},  // SOR�s�v
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
        utility_http::AddFormDataParamToString(PARAM_NAME_LEVERAGE_CATEGORY, L"6", form_data);   // >ToDo< ��ʐM�p/���v��̑Ή�
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
 *  @brief  �M�p�ԍϒ����pFormData�\�z
 *  @param[in]  caIQ
 *  @param[in]  quantity_tag
 *  @param[in]  order           ��������
 *  @param[in]  pass
 *  @param[in]  regist_id       �������j�[�NID(�O�������瓾��)
 *  @param[out] request         �i�[��
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
        { L"caIsLump",          L"false"},  // �ԍό��ʎw����@(=��) �ꊇ�Ȃ�true
        { L"request_type",      L"16"   },  // �ԍϏ���(=�����Â���)
        { L"caLiKbn" ,          L"today"},  // ����(=������)
        { L"limit",             L""     },  // ���Ԏw��(=������)
        { L"cayen.isStopOrder", L"false"},  // �t�����t���O(=�ʏ�) �t�����Ȃ�true
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
    utility_http::AddFormDataParamToString(PARAM_NAME_LEVERAGE_CATEGORY, L"6", form_data);   // >ToDo< ��ʐM�p/���v��̑Ή�
    //
    if (!quantity_tag.empty()) {
        std::wstring quantity_tag_w(std::move(utility_string::ToWstring(quantity_tag)));
        utility_http::AddFormDataParamToString(quantity_tag_w, std::to_wstring(order.m_number), form_data);
        utility_http::AddFormDataParamToString(L"caIQ", caIQ_w, form_data);
        utility_http::AddFormDataParamToString(L"caPSO", L"61", form_data);
        utility_http::AddFormDataParamToString(L"decision", L"���ʊm��", form_data);
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
 *  @brief  ���������pFormData�\�z
 *  @param[in]  order_id    �����ԍ�(�Ǘ��p)
 *  @param[in]  order       ��������
 *  @param[in]  pass
 *  @param[in]  regist_id   �������j�[�NID(�O�������瓾��)
 *  @param[out] request     �i�[��
 */
void BuildCorrectOrderFormData(int32_t order_id, const StockOrder& order, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;

    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.quryOrderStatus", L"order"},  // ???
        { L"torihiki_kbn" ,         L"1"    },  // ����敪(�Ƃ�?)
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
 *  @brief  ����������pFormData�\�z
 *  @param[in]  order_id    �����ԍ�(�Ǘ��p)
 *  @param[in]  pass
 *  @param[in]  regist_id   �������j�[�NID(�O�������瓾��)
 *  @param[out] request     �i�[��
 */
void BuildCancelOrderFormData(int32_t order_id, const std::wstring& pass, int64_t regist_id, web::http::http_request& request)
{
    std::wstring form_data;
    //
    const utility_http::sFormDataParam ORDER_FORM[] = {
        { L"cayen.quryOrderStatus", L"order"},  // ???
        { L"torihiki_kbn" ,         L"1"    },  // ����敪(�Ƃ�?)
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
 *  @brief  responseStockOrderExec�̌��ʂ�RcvResponseStockOrder�֊i�[
 *  @param[in]  t           �X�N���v�g����tuple
 *  @param[out] rcv_order   �i�[��
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
