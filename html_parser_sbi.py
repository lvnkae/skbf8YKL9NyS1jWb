# -*- coding: utf-8 -*-
# for 2.7
 
import urllib2

from HTMLParser import HTMLParser


#   @brief  文字列中の','を全部削除する
#   @param  src 入力文字列
#   @return 削除後の文字列
def deleteComma(src):
    while (',' in src):
        src = src.replace(',','')
    return src

#   @brief  文字列中の'&nbsp;'を全て' 'に置き換える
#   @param  src 入力文字列
#   @return 削除後の文字列
def deleteNBSP(src):
    while ('&nbsp' in src):
        src = src.replace('&nbsp',' ')
    return src
    
#   @brief  文字列中の' 'を全部削除する
#   @param  src 入力文字列
#   @return 削除後の文字列
def deleteSpace(src):
    while (' ' in src):
        src = src.replace(' ','')
    return src

#   @brief  文字列をスペース・改行で分割
#   @param  src 入力文字列
def splitBySPCRLF(src):
    ret_str = []
    div_lf = src.split('\n');
    for str_div_lf in div_lf:
        if str_div_lf:
            div_sp = str_div_lf.split(' ')
            for str_div_sp in div_sp:
                if str_div_sp:
                    if not str_div_sp == '\r':
                        ret_str.append(str_div_sp)
    return ret_str

#   @brief  listからkeyを探してindexを返す
#   @param  src 対象list
#   @param  key 探すkey
def searchTdDataTag(src, key):
    key_inx = 0
    for tag in src:
        if tag == key:
            return key_inx
        key_inx += 1
    return -1

#   @brief  listからkeyを探してindexを返す
#   @param  src 対象list
#   @param  inx 参照するlist要素のindex
#   @param  key 探すkey
def searchTdDataTag_index(src, inx, key):
    key_inx = 0
    for elem in src:
        if elem[inx] == key:
            return key_inx
        #else:
         #   print elem[inx].decode('utf-8')
        key_inx += 1
    return -1
    
#   @brief  取引所名からSBI用のタグに変換
#   @param  str 取引所名文字列    
def getInvestimentSBITag(str):
    if u'東証' in str.decode('utf-8'):
        return 'TKY'
    elif u'PTS' in str.decode('utf-8'):
        return 'JNX'
    else:
        return ''

#   @brief  C++のeOrderTypeカバーclass
class eOrderTypeEnum:

    def __init__(self):
       self.eOrderType = { '買':1, '売':2, '訂正':3, '取消':4, '返売':5, '返買':6 }
       self.eOrderType2 = { '現物買':1, '現物売':2, '信新買':1, '信新売':2, '信返売':5, '信返買':6 }
    
    # SBI取引種別文字列からeOrderType(数値)を得る
    def getOrderTypeFromStr(self, str):
        for key, value in self.eOrderType2.iteritems():
            if key in str:
                b_leverage = not '現物' in key
                return (b_leverage, value)
        return (False, 0)

    # タグからeOrderType(数値)を得る
    def getOrderType(self, tag):
        return self.eOrderType[tag]
    
    # eOrderType(数値)からタグを得る
    def getOrderTag(self, order_type):
        for key, value in self.eOrderType.iteritems():
            if value == order_type:
                return key
        return ''

 
#   @brief  ログインresponseをparseするclass
#   @note   SBI-mobile[バックアップ]サイト用
class LoginParserMobile(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.start_tag = ''
        #
        self.b_result = False       # parse成否
        self.login_result = False   # ログイン成否
 
    def handle_starttag(self, tag, attrs):
        if not self.b_result:
            self.start_tag = tag
        
    def handle_endtag(self, tag):
        if not self.b_result:
            self.start_tag = ''
            
    def unescape(self, s):
        # attrsに'&nbsp;'が含まれてるとエラーを出すので置換して対処
        return HTMLParser.unescape(self, deleteNBSP(s))
 
    def handle_data(self, data): # 要素内用を扱うためのメソッド
        if not self.b_result:
            if self.start_tag == 'font':
                if u'パスワードが違います' in data.decode('utf-8'):
                    self.b_result = True
                    self.login_result = False
            elif self.start_tag == 'a':
                if u'ログアウト' in data.decode('utf-8'):
                    self.b_result = True
                    self.login_result = True

#   @brief  ログイン結果を得る(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 ログインresponse(html/utf-8)
#   @return (処理成否, ログイン成否)
def responseLoginMobile(html_u8):

    parser = LoginParserMobile()
    parser.feed(html_u8)
    parser.close()
    
    return (parser.b_result, parser.login_result)

    
#   @brief  ログインresponseをparseするclass
#   @note   SBI-PC[メイン]サイト用
class LoginParserPC(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.login_ok = False
        self.login_fail = False
        self.important_msg = False
        self.check_important_seq = 0
 
    def handle_starttag(self, tag, attrs):
        if self.login_ok:
            # ログイン成功確認済み →「重要なお知らせ」が届いてないか調べる
            if not self.important_msg:
                # <div class = "title-text>"><b>重要なお知らせ</b></div>を想定
                if tag == 'div':
                    attrs = dict(attrs)
                    if 'class' in attrs:
                        if attrs['class'] == 'title-text':
                            self.check_important_seq = 1
                elif tag == 'b':
                    attrs = dict(attrs)
                    if self.check_important_seq == 1:
                        self.check_important_seq = 2
                else:
                    self.check_important_seq = 0
        else:
            if not self.login_fail:
                # ログアウトボタン(gif)があればログイン成功
                # ログインボタン(gif)があればログイン失敗
                if tag == 'img':
                    attrs = dict(attrs)
                    if 'alt' in attrs:
                        u8_alt = attrs['alt'].decode('utf-8')
                        if u'ログアウト' == u8_alt:
                            self.login_ok = True
                        elif u'ログイン' in u8_alt:
                            self.login_fail = True
                    elif 'title' in attrs:
                        u8_title = attrs['title'].decode('utf-8')
                        if u'ログアウト' in u8_title:
                            self.login_ok = True
                        elif u'ログイン' in u8_title:
                            self.login_fail = True

    def handle_data(self, data):
        if self.check_important_seq == 2:
            self.check_important_seq = 0
            if u'重要なお知らせ' in data.decode('utf-8'):
                self.important_msg = True

#   @brief  ログイン結果を得る(SBI-PC[メイン]サイト用)
#   @param  html_sjis   ログインresponse(html/Shift-JIS)
#   @return (処理成否, ログイン成否, 重要なお知らせ有無)
def responseLoginPC(html_sjis):

    parser = LoginParserPC()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))

    if parser.login_ok:
        # parse成功/ログイン成功
        parser_result = True
        login_result = True
        important_msg = parser.important_msg
    elif parser.login_fail:
        # parse成功/ログイン失敗(uid/pwdエラー)
        parser_result = True
        login_result = False
        important_msg = False
    else:
        # parse失敗/ログイン失敗(parseエラー)
        parser_result = False
        login_result = False
        important_msg = False

    parser.close()
    
    return (parser_result, login_result, important_msg)

    
#   @brief  regist_idを切り出すclass
#   @note   SBI-mobile[バックアップ]サイト<汎用>
class RegistIDParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.b_ok = False       #意図したhtmlであることが確認できたらTrue
        self.tag_now = ''       #注目タグ(子class用)
        self.b_start = False    #開始タグだったらTrue(子class用)
        #
        self.regist_id = -1L
 
    def handle_starttag(self, tag, attrs):
        self.tag_now = tag
        self.b_start = True
        # 意図したhtmlであることが確認できてない、またはregist_id確保済みなら処理しない
        if self.b_ok and self.regist_id <= 0:
            if tag == 'input':
                attrs = dict(attrs)
                if 'name' in attrs and 'value' in attrs:
                    if attrs['name'] == 'regist_id':
                        self.regist_id = long(attrs['value'])


#   @brief  ポートフォリオ切り出しclass
#   @note   SBI-mobile[バックアップ]サイト用
#   @note   lumpStockEntryExecute.doのresponseでしか検証してない
class PortfolioParserMobile(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.now_code = 0
        self.now_tag = '';
        self.portfolio = []
 
    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            attrs = dict(attrs)
            if 'href' in attrs:
                if 'ipm_product_code' in attrs['href']:
                    self.now_code = int(attrs['href'][-4:])

    def handle_data(self, data):
        if not self.now_code == 0:
            self.portfolio.append([data, self.now_code])
            self.now_code = 0

    
#   @brief  ポートフォリオを切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 ポートフォリオを含むresponse(html/utf-8)
#   @return list[銘柄名, 銘柄コード]
#   @note   主にlumpStockEntryExecute.doのresponseをparseする
def getPortfolioMobile(html_u8):

    parser = PortfolioParserMobile()
    parser.feed(html_u8)
    parser.close()

    return parser.portfolio


#   @brief  ポートフォリオ切り出しclass
#   @note   SBI-PC[メイン]サイト用/監視銘柄
class PortfolioParserPC(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.now_tag = ''
        self.is_portfolio = False       #以降にPFを含むhtmlだと判明したらTrue
        self.is_prev_pf_table = False   #PFテーブルの直前だったらTrue
        self.in_pf_table = False        #PFテーブルに入ったらTrue
        self.in_pf_row = False          #PFテーブルの1銘柄分の行に入ったらTrue
        self.in_pf_obj = False          #PFテーブルの1銘柄分の行の1要素に入ったらTrue
        self.is_end = False             #終了タグ処理中
        self.current_obj = ''           #現在注目している要素(テキスト)
        self.pf_obj_name = []           #項目名リスト
        self.pf_row_inx = 0
        self.pf_obj_inx = 0
        self.portfolio = []
        self.valueunit = [ 0, 0, 0, 0, 0, 0, long(0) ]
        self.is_proc_end = False
 
    def handle_starttag(self, tag, attrs):
        if self.is_proc_end:
            return
        self.now_tag = tag
        self.is_end = False
        if not self.is_portfolio:
            if tag == 'select':
                attrs = dict(attrs)
                if 'name' in attrs:
                    if attrs['name'] == 'portforio_id':
                        self.is_portfolio = True
        elif not self.is_prev_pf_table:
            if tag == 'input':
                attrs = dict(attrs)
                if 'value' in attrs:
                    if u'情報更新' == attrs['value'].decode('utf-8'):
                        self.is_prev_pf_table = True
        elif not self.in_pf_table:
            if tag == 'table':
                self.in_pf_table = True
        elif not self.in_pf_row:
            if tag == 'tr':
                self.in_pf_row = True
                self.pf_obj_inx = 0
        elif not self.in_pf_obj:
            if tag == 'td':
                self.in_pf_obj = True
                self.current_obj = ''
                        
    def isNumber(self, src):
        length = len(src)
        for c in src:
            if not c.isdigit():
                if not c == ',' and not c == '.':
                    return False
        return True

    def handle_endtag(self, tag):
        if self.is_proc_end:
            return
        self.is_end = True
        if self.in_pf_row:
            if tag == 'tr':
                #PFテーブル1行分終わり
                self.in_pf_row = False
                if not self.pf_row_inx == 0: #項目名行以外
                    self.portfolio.append(self.valueunit[:])
                self.pf_row_inx += 1
            elif tag == 'td' and self.in_pf_obj:
                #PFテーブル1obj分終わり
                self.in_pf_obj = False 
                if self.pf_row_inx == 0: #項目名行
                    self.pf_obj_name.append(self.current_obj)
                else:
                    obj_name = self.pf_obj_name[self.pf_obj_inx].decode('utf-8')
                    if u'銘柄' in obj_name:
                        self.valueunit[0] = int(self.current_obj)
                    elif u'現在値' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[1] = float(self.current_obj)
                        else:
                            self.valueunit[1] = float(-1.0)
                    elif u'出来高' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[6] = long(self.current_obj)
                        else:
                            self.valueunit[6] = long(0)
                    elif u'始値' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[2] = float(self.current_obj)
                        else:
                            self.valueunit[2] = float(-1.0)
                    elif u'高値' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[3] = float(self.current_obj)
                        else:
                            self.valueunit[3] = float(-1.0)
                    elif u'安値' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[4] = float(self.current_obj)
                        else:
                            self.valueunit[4] = float(-1.0)
                    elif u'前日終値' in obj_name:
                        if self.isNumber(self.current_obj):
                            self.valueunit[5] = float(self.current_obj)
                        else:
                            self.valueunit[5] = float(-1.0)
                self.pf_obj_inx += 1
        elif self.in_pf_table:
            if tag == 'table':
                self.in_pf_table = False
                self.is_proc_end = True
                
    def handle_data(self, data):
        if self.is_proc_end:
            return
        if self.in_pf_obj and not self.is_end:
            if len(self.current_obj) == 0:
                self.current_obj = deleteComma(data)

    
#   @brief  ポートフォリオを切り出す(SBI-PC[メイン]サイト用)/監視銘柄
#   @param  html_sjis   ポートフォリオを含むresponse(html/Shift-JIS)
#   @return list[銘柄コード, 現値, 始値, 高値, 安値, 前日終値, 出来高]
def getPortfolioPC(html_sjis):

    #strbuff = open('portfolio_get.html').read()

    parser = PortfolioParserPC()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    parser.close()
    
    return parser.portfolio
    
#   @brief  ポートフォリオ切り出しclass
#   @note   SBI-PC[メイン]サイト用/保有銘柄
class PortfolioParserPC_Owned(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.now_tag = ''
        self.b_start = False    #開始タグ処理中
        self.current_obj = ''
        self.tr_data = []
        self.td_data = []
        self.b_spot = False     # 現物収集開始フラグ
        self.b_lev = False      # 信用収集開始フラグ
        self.data_tag = ''
        self.b_start_tr = False
        self.tr_inx = 0
        self.td_inx = 0
        self.td_rowspan = 0
        self.parse_step = 0 #parse段階 0:対象divタグ走査
                            #          1:divタグ終了チェック => テキストデータチェック(=取得成否チェック)して2へ
                            #          2:対象Bタグ走査 (保有株データ位置検出)
                            #          3:table走査 (見つからずにtr～/trまで走査しきったら空とみなして2へ)
                            #          4:trタグ走査 (table終了検出したら保有株構築して2へ)
                            #          5:tdタグ走査 (trタグ終了検出したら4へ)
                            #          6:tdタグ終了チェック => テキストデータ取得して5へ
                            #         -1:完了
        self.b_result = False   # parse成否
        self.spot_owned = []    # 現物株
        self.lev_owned = []     # 信用建玉
 
    def handle_starttag(self, tag, attrs):
        if self.parse_step >= 0:
            self.b_start = True
            self.now_tag = tag
            if self.parse_step == 0:
                if tag == 'div':
                    attrs = dict(attrs)
                    if 'class' in attrs and attrs['class'] == 'title-text':
                        #print 'pserse0->1'
                        self.parse_step = 1
            elif self.parse_step == 2:
                if self.b_spot and self.b_lev:
                    self.parse_step = -1 #終わり
            elif self.parse_step == 3:
                if tag == 'tr':
                    self.b_start_tr = True
                elif tag == 'table':
                    self.b_start_tr = False
                    self.tr_inx = 0
                    self.parse_step = 4
            elif self.parse_step == 4:
                if tag == 'tr':
                    self.td_inx = 0
                    self.parse_step = 5
            elif self.parse_step == 5:
                if tag == 'td':
                    attrs = dict(attrs)
                    if 'rowspan' in attrs:
                        self.td_rowspan = int(attrs['rowspan'])
                    else:
                        self.td_rowspan = 0
                    self.parse_step = 6
                        
    def handle_data(self, data):
        if self.parse_step >= 0:
            if self.parse_step == 1 or self.parse_step == 6:
                if data:
                    self.current_obj += deleteNBSP(data)
            elif self.parse_step == 2:
                if self.now_tag == 'b':
                    u8data = data.decode('utf-8')
                    if u'株式' in u8data:
                        if u'現物' in u8data and u'一般' in u8data:
                            self.data_tag = '現物'
                            self.b_spot = True
                            self.parse_step = 3
                            #print 'pserse2->3A'
                        elif u'信用' in u8data:
                            self.data_tag = '信用'
                            self.b_lev = True
                            self.parse_step = 3
                            #print 'pserse2->3B'
            self.b_start = False

    def handle_endtag(self, tag):
        if self.parse_step >= 0:
            if self.parse_step == 1:
                if tag == 'div':
                    if self.current_obj.decode('utf-8') == u'ポートフォリオ':
                        #print 'pserse1->2'
                        self.current_obj = ''
                        self.b_result = True
                        self.parse_step = 2
                    else:
                        #print 'pserse1->-1'
                        self.parse_step = -1
            elif self.parse_step == 3:
                if tag == 'tr' and self.b_start_tr:
                    # 収集対象保有株が空だったので次へ
                    self.b_start_tr = False
                    self.parse_step == 2
            elif self.parse_step == 4:
                if tag == 'table':
                    if self.data_tag == '現物':
                        if self.tr_inx > 1:
                            len_tr_data = len(self.tr_data)
                            code = 0
                            for tr_inx in range(1, len_tr_data):
                                t_td_data = self.tr_data[tr_inx]
                                code_str = t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '銘柄（コード）')][0]
                                if code_str:
                                    code = int(code_str[0:4])
                                if code == 0:
                                    self.parse_step = -1
                                    return #error
                                in_spot_inx = -1
                                len_spot_data = len(self.spot_owned)
                                for spot_inx in range(0, len_spot_data):
                                    if self.spot_owned[spot_inx][0] == code:
                                        in_spot_inx = spot_inx
                                        break
                                spot_number = int(deleteComma(t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '数量')][0]))
                                if in_spot_inx >= 0:
                                    old_number = self.spot_owned[in_spot_inx][1];
                                    self.spot_owned[in_spot_inx] = (code, old_number + spot_number)
                                else:
                                    self.spot_owned.append((code, spot_number))
                    elif self.data_tag == '信用':
                        if self.tr_inx > 1:
                            len_tr_data = len(self.tr_data)
                            code = 0
                            for tr_inx in range(1, len_tr_data):
                                t_td_data = self.tr_data[tr_inx]
                                code_str = t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '銘柄（コード）')][0]
                                if code_str:
                                    code = int(code_str[0:4])
                                if code == 0:
                                    self.parse_step = -1
                                    return #error
                                buysell_str = t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '売買建')][0]
                                sell_flag = False
                                if buysell_str == '売建':
                                    sell_flag = True
                                self.lev_owned.append((code,
                                                       int(deleteComma(t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '数量')][0])),
                                                       sell_flag,
                                                       t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '買付日')][0],
                                                       float(deleteComma(t_td_data[searchTdDataTag_index(self.tr_data[0], 0, '建単価')][0]))
                                                     ))
                    del self.tr_data[:]
                    self.parse_step = 2
            elif self.parse_step == 5:
                if tag == 'tr':
                    self.tr_data.append(self.td_data[:])
                    del self.td_data[:]
                    self.tr_inx += 1
                    self.parse_step = 4
            elif self.parse_step == 6:
                if tag == 'td':
                    if self.tr_inx > 0:
                        t_td_data = self.tr_data[self.tr_inx-1]
                        len_td_data = len(t_td_data)
                        # 前行のrowspanを考慮する
                        # (rowspanが2以上ならば今行は空データを突っ込む)
                        for td_inx in range(self.td_inx, len_td_data):
                            t_rowspan = t_td_data[td_inx][1]
                            if t_rowspan >= 2:
                                self.td_data.append(('', t_rowspan-1))
                                self.td_inx += 1
                            else:
                                break
                    # rowspanの影響がない列へ挿入
                    self.td_data.append((self.current_obj, self.td_rowspan))
                    self.td_inx += 1
                    self.parse_step = 5
                    self.current_obj = ''
    
#   @brief  ポートフォリオを切り出す(SBI-PC[メイン]サイト用)/保有銘柄
#   @param  html_sjis   ポートフォリオを含むresponse(html/Shift-JIS)
#   @return list[銘柄コード, 数量], list[銘柄コード, 数量, 売買フラグ, 建日, 建単価]
def getPortfolioPC_Owned(html_sjis):

    #strbuff = open('portfolio.html').read()

    parser = PortfolioParserPC_Owned()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    #parser.feed(strbuff.decode('cp932').encode('utf-8'))
    parser.close()
    
    return (parser.b_result,
            parser.spot_owned,
            parser.lev_owned)


#   @brief  ポートフォリオ登録確認応答からregist_idを切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 lumpStockEntryConfirmのresponse(html/utf-8)
#   @return regist_id
def getPortfolioRegistID(html_u8):

    parser = RegistIDParser()
    parser.b_ok = True #ノーチェッkでTrueにしとく
    parser.feed(html_u8)
    regist_id = parser.regist_id
    parser.close()
    
    return regist_id


#   @brief  ポートフォリオ転送トップページ取得結果(SBI-PC[メイン]サイト用)
#   @param  html_sjis   ログインresponse(html/Shift-JIS)
#   @return 成否 
class EntranceOfPortfolioTransmissionParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.b_success = False
 
    def handle_starttag(self, tag, attrs):
        if not self.b_success:
            # 「登録銘柄リストの追加・置き換え機能を利用する」ボタン(gif)があれば取得成功
            if tag == 'img':
                attrs = dict(attrs)
                if 'alt' in attrs:
                    if u'登録銘柄リストの追加・置き換え機能' in attrs['alt'].decode('utf-8'):
                        self.b_success = True

def responseGetEntranceOfPortfolioTransmission(html_sjis):

    parser = EntranceOfPortfolioTransmissionParser()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    parser.close()
    
    return parser.b_success


#   @brief  ポートフォリオ転送要求結果(SBI-PC[メイン]サイト用)
#   @param  html_sjis   ログインresponse(html/Shift-JIS)
#   @return 成否 
class ReqPortfolioTransmissionParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.now_tag = ''
        self.b_start = False
        self.b_success = False
 
    def handle_starttag(self, tag, attrs):
        self.b_start = True
        if not self.b_success:
            self.now_tag = tag
            
    def handle_data(self, data):
        if not self.b_success and self.b_start:
            if self.now_tag == 'p':
                if u'送信指示予約を受付ました' in data.decode('utf-8'):
                    self.b_success = True
        self.b_start = False
    
def responseReqPortfolioTransmission(html_sjis):

    parser = ReqPortfolioTransmissionParser()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    parser.close()
    
    return parser.b_success
    
    
#   @brief  当日約定履歴切り出しclass
#   @note   SBI-PC[メイン]サイト用/保有銘柄
#   @return list[注文番号(表示用), 注文種別(eOrderType), 取引所種別str, 銘柄コード, 信用フラグ, 完了フラグ, list[約定年月日str, 単価, 株数]
class TodayExecInfoParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.now_tag = ''
        self.b_start = False    #開始タグ処理中
        self.current_obj = ''
        self.table_count = 0
        self.header_tag = []
        self.data_tag = []
        self.header_work = []
        self.data_work = []
        self.data_list_work = []
        self.b_read_header = False
        self.tr_inx = 0
        self.td_inx = 0
        self.parse_step = 0 #parse段階 0:対象divタグ走査
                            #          1:divタグ終了チェック => テキストデータチェック(=取得成否チェック)して2へ
                            #          2:table走査 (データ本体[10番目のtable]を見つけたら3へ)
                            #          3:trタグ走査 (tag=4tr,data=header<2tr>+data<N*1tr>。table終了検出したら完了[-1]。)
                            #          4:tdタグ走査 (trタグ終了検出したら3へ)
                            #          5:tdタグ終了チェック => テキストデータ取得して4へ
                            #         -1:完了
        self.b_result = False   # parse成否
        self.exec_info = []
        
    def OutputWork(self):
        if self.b_read_header:
            if len(self.header_tag) == len(self.header_work) and self.data_list_work:
                odtype_str = self.header_work[searchTdDataTag(self.header_tag, '取引預り/手数料')]
                ret_func = eOrderTypeEnum().getOrderTypeFromStr(odtype_str)
                order_state = self.header_work[searchTdDataTag(self.header_tag, '発注状況')]
                b_complete = '完了' in order_state
                brand = self.header_work[searchTdDataTag(self.header_tag, '銘柄コード市場')]
                brand_list = splitBySPCRLF(brand)
                self.exec_info.append((int(self.header_work[searchTdDataTag(self.header_tag, '注文番号')]),
                                       ret_func[1],
                                       getInvestimentSBITag(brand_list[2]),
                                       int(brand_list[1]),
                                       ret_func[0],
                                       b_complete,
                                       self.data_list_work[:]))      
            del self.header_work[:]
            del self.data_list_work[:]
            
    def AppendDataWorkToDataListWork(self):
        datetime = self.data_work[searchTdDataTag(self.data_tag, '約定日時')]
        if not len(datetime) == 13: # 'MM/DDHH:MM:SS'必須
            return False
        datetime = datetime[0:5] + ' ' + datetime[5:] #MM/DDとHH:MM:SSの間にスペースを入れる
        self.data_list_work.append((datetime,
                                    int(deleteComma(self.data_work[searchTdDataTag(self.data_tag, '約定株数')])),
                                    float(deleteComma(self.data_work[searchTdDataTag(self.data_tag, '約定単価')]))))
        del self.data_work[:]
        return True
 
    def handle_starttag(self, tag, attrs):
        if self.parse_step >= 0:
            self.b_start = True
            self.now_tag = tag
            if self.parse_step == 0:
                if tag == 'div':
                    attrs = dict(attrs)
                    if 'class' in attrs and attrs['class'] == 'title-text':
                        #print 'pserse0->1'
                        self.parse_step = 1
            elif self.parse_step == 2:
                if tag == 'table':
                    self.table_count += 1
                    if self.table_count == 10:
                        #print 'pserse2->3'
                        self.parse_step = 3
            elif self.parse_step == 3:
                if tag == 'tr':
                    #print 'pserse3->4'
                    self.parse_step = 4
            elif self.parse_step == 4:
                if tag == 'td':
                    #print 'pserse4->5'
                    self.parse_step = 5
                        
    def handle_data(self, data):
        if self.parse_step >= 0:
            if self.parse_step == 1 or self.parse_step == 5:
                if data:
                    self.current_obj += data
            elif self.parse_step == 2:
                if self.now_tag == 'div' or self.now_tag == 'b' or self.now_tag == 'font':
                    if u'お客様の株式注文はございません' in data.decode('utf-8'):
                        #print 'none'
                        self.parse_step = -1
            self.b_start = False

    def handle_endtag(self, tag):
        if self.parse_step >= 0:
            if self.parse_step == 1:
                if tag == 'div':
                    if self.current_obj.decode('utf-8') == u'注文一覧':
                        #print 'pserse1->2'
                        self.current_obj = ''
                        self.b_result = True
                        self.parse_step = 2
                    else:
                        #print 'pserse1->-1'
                        self.parse_step = -1
            elif self.parse_step == 3:
                if tag == 'table':
                    self.OutputWork()
                    self.parse_step == -1 # 完了
            elif self.parse_step == 4:
                if tag == 'tr':
                    self.td_inx = 0
                    #print 'pserse4->3'
                    self.parse_step = 3
                    if not self.b_read_header:
                        if self.tr_inx == 3:
                            self.b_read_header = True
                            self.tr_inx = 0
                        else:
                            self.tr_inx += 1
                    elif self.tr_inx >= 2:
                        if len(self.data_tag) == len(self.data_work):
                            self.AppendDataWorkToDataListWork()
                        else:
                            #print len(self.data_tag),len(self.data_work)
                            #print self.data_tag,self.data_work
                            self.parse_step = -1 #error(タグとデータが食い違ってる)
                        self.tr_inx += 1
                    else:
                        self.tr_inx += 1
            elif self.parse_step == 5:
                if tag == 'td':
                    if not self.b_read_header:
                        if self.tr_inx == 0 or self.tr_inx == 1:
                            self.header_tag.append(deleteNBSP(self.current_obj))
                        elif self.tr_inx == 3:
                            self.data_tag.append(deleteNBSP(self.current_obj))
                    else:
                        if self.td_inx == 0 and self.tr_inx >= 2:
                            # 「約定データtr」かチェック
                            if not self.current_obj.decode('utf-8') == u'約定':
                                self.tr_inx = 0
                                self.OutputWork()
                        if self.tr_inx == 0 or self.tr_inx == 1:
                            self.header_work.append(self.current_obj)
                        else:
                            self.data_work.append(deleteNBSP(self.current_obj))
                    self.td_inx += 1
                    self.current_obj = ''
                    #print 'pserse5->4'
                    self.parse_step = 4
    
#   @brief  当日注文一覧から約定履歴を切り出す(SBI-PC[メイン]サイト用)
#   @param  html_sjis   当日注文一覧response(html/Shift-JIS)
#   @return list[注文番号(表示用), 注文種別(eOrderType), 取引所種別str, 銘柄コード, 信用フラグ, 完了フラグ, list[約定年月日str, 単価, 株数]
def getTodayExecInfo(html_sjis):

    #strbuff = open('order_list4.html').read()

    parser = TodayExecInfoParser()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    #parser.feed(strbuff.decode('cp932').encode('utf-8'))
    parser.close()
    
    return (parser.b_result, parser.exec_info)
    

#   @brief  余力を切り出すclass
#   @note   SBI-mobile[バックアップ]サイト<汎用>
class MarginMobileParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.b_result = False
        self.tag_now = ''
        self.b_start = False
 
    def handle_starttag(self, tag, attrs):
        if not self.b_result:
            self.tag_now = tag
            self.b_start = True
        
    def handle_data(self, data):
        if not self.b_result:
            if self.b_start:
                if self.tag_now == 'title':
                    if u'信用建余力' in data.decode('utf-8'):
                        self.b_result = True

#   @brief  余力を切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 余力画面のresponse(html/utf-8)
#   @return result
def getMarginMobile(html_u8):

    parser = MarginMobileParser()
    parser.feed(html_u8)
    parser.close()
    
    return parser.b_result


#   @brief  regist_idを切り出すclass
#   @note   買注文入力画面(SBI-mobile[バックアップ]サイト)用
class StockOrderRegistIDParser(RegistIDParser):
 
    def __init__(self, title_str, order_type):
        RegistIDParser.__init__(self)
        self.title_str = title_str
        self.order_tag = eOrderTypeEnum().getOrderTag(order_type)

    def handle_data(self, data):
        if not self.b_ok and self.b_start and self.tag_now == 'title':
            if '買' == self.order_tag or '返買' == self.order_tag:
                u8data = data.decode('utf-8')
                if self.title_str in u8data:
                    # 括弧の全/半角に表記ゆれがある…
                    if u'買)' in u8data or u'買）' in u8data or u'買PTS)' in u8data:
                        self.b_ok = True
            elif '売' == self.order_tag or '返売' == self.order_tag:
                u8data = data.decode('utf-8')
                if self.title_str in u8data:
                    # 括弧の全/半角に表記ゆれがある…
                    if u'売)' in u8data or u'売）' in u8data or u'売PTS)' in u8data:
                        self.b_ok = True
            elif '訂正' == self.order_tag:
                if u'注文訂正' in data.decode('utf-8'):
                        self.b_ok = True
            elif '取消' == self.order_tag:
                if u'注文取消' in data.decode('utf-8'):
                        self.b_ok = True
        self.b_start = False

#   @brief  regist_idを切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 buyOrderEntry/sellOrderEntryのresponse(html/utf-8)
#   @return regist_id
def getStockOrderRegistID(html_u8, order_type):

    parser = StockOrderRegistIDParser(u'注文入力', order_type)
    parser.feed(html_u8)
    parser.close()
    return parser.regist_id
        
#   @brief  regist_idを切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 buyOrderEntryConfirm/sellOrderEntryConfirmのresponse(html/utf-8)
#   @return regist_id
def getStockOrderConfirmRegistID(html_u8, order_type):

    parser = StockOrderRegistIDParser(u'注文確認', order_type)
    parser.feed(html_u8)
    parser.close()
    return parser.regist_id


#   @brief  注文結果切り出しclass
#   @note   注文受付画面(SBI-mobile[バックアップ]サイト)用
class StockOrderExParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.tag_now = ''
        self.b_start = False
        self.order_type_tag = ''
        self.str_work = ''
        self.td_data = []
        self.table_count = 0
        self.parse_step = 0 #parse段階 0:対象titleタグ走査
                            #          1:タイトルチェック
                            #          2:対象brタグ走査
                            #          3:注文成否フラグセット・注文種別セット
                            #          4:対象inputタグ走査、注文番号(管理用/表示用)取得
                            #          5:対象table走査
                            #          6:tdタグ走査
                            #            + tableタグを抜けて...
                            #               table2個分走査し終えてたらtdデータを変換して処理完了
                            #               まだなら5に戻す
                            #          7:tdタグ終了までのデータ取得(空でないものが見つかるまで)
                            #            + tdタグを抜けたら6に戻す
                            #         -1:完了
        self.b_result = False   #注文成否
        self.b_leverage = False #信用フラグ
        self.order_type = 0     #注文種別[eOrderType]
        self.order_id = -1      #注文番号(内部値/管理用)
        self.user_order_id = -1 #注文番号(ユーザ固有/表示用)
        self.code = 0           #銘柄コード
        self.investments = ''   #取引所タグ
        self.numbers = 0        #枚数
        self.value = 0.0        #価格
 
    def handle_starttag(self, tag, attrs):
        if self.parse_step >= 0:
            self.tag_now = tag
            self.b_start = True
            if self.parse_step == 0:
                if tag == 'div':
                    attrs = dict(attrs)
                    if 'class' in attrs and attrs['class'] == 'titletext':
                        self.parse_step = 1
            elif self.parse_step == 2:
                if tag == 'br':
                    self.parse_step = 3
            elif self.parse_step == 4:
                # 買/売ならばここで注文番号(管理用)を得てから次ステップ(5)へ
                if tag == 'input':
                    attrs = dict(attrs)
                    if 'name' in attrs and 'value' in attrs and attrs['name'] == 'orderNum':
                        self.parse_step = 5
                        self.order_id = int(attrs['value'])
            elif self.parse_step == 5:
                if tag == 'table':
                    if self.table_count < 2:
                        self.parse_step = 6
                        self.table_count += 1
            elif self.parse_step == 6:
                if tag == 'td':
                    self.parse_step = 7
                    self.str_work = ''
                        
    def handle_data(self, data):
        if self.parse_step >= 0:
            if self.parse_step == 4:
                if self.tag_now == 'br':
                    if data.isdigit():
                        self.user_order_id = int(data)
                        # 取消/訂正ならばここで次ステップ(5)へ進める(htmlに管理用注文番号がないので)
                        if self.order_type_tag == '取消' or self.order_type_tag == '訂正':
                            self.parse_step = 5
            if self.b_start:
                if self.parse_step == 1:
                    if self.tag_now == 'div':
                        u_str = data.decode('utf-8')
                        if u'注文受付' in u_str:
                            self.parse_step = 2
                            if u'返済売' in u_str:
                                self.order_type_tag = '返売'
                            elif u'返済買' in u_str:
                                self.order_type_tag = '返買'
                            elif u'買' in u_str:
                                self.order_type_tag = '買'
                            elif u'売' in u_str:
                                self.order_type_tag = '売'
                        elif u'注文訂正' in u_str and u'受付' in u_str:
                            self.parse_step = 2
                            self.order_type_tag = '訂正'
                        elif u'注文取消' in u_str and u'受付' in u_str:
                            self.parse_step = 2
                            self.order_type_tag = '取消'
                elif self.parse_step == 3:
                    if self.tag_now == 'br':
                        u_str = data.decode('utf-8')
                        # ひらがな/漢字に表記ゆれがある…
                        if u'注文' in u_str and (u'受付いたしました' in u_str or u'受付致しました' in u_str):
                            self.parse_step = 4
                            self.b_result = True
                            eot = eOrderTypeEnum()
                            self.order_type = eot.getOrderType(self.order_type_tag)
                elif self.parse_step == 7:
                    if self.str_work == '':
                        self.str_work = data
            self.b_start = False

    def handle_endtag(self, tag):
        if self.parse_step >= 0:
            if self.parse_step == 6:
                if tag == 'table':
                    if self.table_count < 2:
                        self.parse_step = 5
                    else:
                        self.parse_step = -1
                        #
                        td_data_len = len(self.td_data)
                        for inx in range(0, td_data_len):
                            # 銘柄コード[0]
                            if inx == 0:
                                self.code = int(self.td_data[inx])
                            # 取引所種別[0]
                            elif inx == 1:
                                self.investments = getInvestimentSBITag(self.td_data[inx])
                            # 2以降は偶数がタグ/奇数が値
                            elif not inx & 1:
                                u_data_tag = self.td_data[inx].decode('utf-8')
                                u_data_value = self.td_data[inx+1].decode('utf-8')
                                if u'株数' in u_data_tag or u'注文数' in u_data_tag:
                                    if not u'注文後売却' in u_data_tag: #要らんやつ除外
                                        self.numbers = int(deleteComma(u_data_value.replace(u'株', '')))
                                if u'価格' in u_data_tag:
                                    if u_data_value == u'成行':
                                        self.value = float(-1.0)
                                    else:
                                        if '/' in u_data_value:
                                            # '不成/468円' みたいなの(価格指定+条件付き発注)
                                            self.value = float(deleteComma(u_data_value.split('/')[1].replace(u'円', '')))
                                        else:
                                            self.value = float(deleteComma(u_data_value.replace(u'円', '')))
                                if u'取引' in u_data_tag:
                                    if u'信用' in u_data_value:
                                        self.b_leverage = True
                                    else:
                                        self.b_leverage = False
            elif self.parse_step == 7:
                if tag == 'td':
                    self.parse_step = 6
                    self.td_data.append(self.str_work)

    
#   @brief  注文結果を切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 buyOrderEx.do/sellOrderEx.do/orderCancelEx.doのresponse(html/utf-8)
#   @return (解析結果, 注文番号(管理用), 注文番号(表示用), 取引所コード(str), 銘柄コード(int), 株数, 価格, 信用フラグ, eOrderType)
def responseStockOrderExec(html_u8):

    parser = StockOrderExParser()
    parser.feed(html_u8)
    parser.close()
    
    return (parser.b_result,
            parser.order_id,
            parser.user_order_id,
            parser.investments,
            parser.code,
            parser.numbers,
            parser.value,
            parser.b_leverage,
            parser.order_type)


#   @brief  返済建玉リスト切り出しclass
#   @note   注文受付画面(SBI-mobile[バックアップ]サイト)用
class RepOrderTateListParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.tag_now = ''
        self.b_start = False
        self.str_work = ''
        self.tr_data = []
        self.td_data = []
        self.quantity = ''
        self.table_count = 0
        self.parse_step = 0 # 0:title走査(成否チェック)
                            # 1:input走査(caIQ,code取得)
                            # 2:対象brタグ走査('一括指定は')
                            # 3:対象table走査(3個目)
                            # 4:tr走査、table終了ならデータ選別して完了(-1)
                            # 5:td走査、tr終了なら4へ
                            # 6:td終了ならdata取得して5へ、同時にinputも走査(quantityタグ取得)
                            #-1:完了
        self.b_result = False   #注文成否
        self.code = 0           #銘柄コード
        self.caIQ = ''          #caIQ
        self.tatedama = []      #建玉情報list
        
 
    def handle_starttag(self, tag, attrs):
        if self.parse_step >= 0:
            self.tag_now = tag
            self.b_start = True
            if self.parse_step == 1:
                if tag == 'input':
                    attrs = dict(attrs)
                    if 'name' in attrs and 'value' in attrs:
                        if attrs['name'] == 'caIQ':
                            self.caIQ = attrs['value']
                        elif attrs['name'] == 'brand_cd':
                            self.code = int(attrs['value'])
                        if self.caIQ and self.code > 0:
                            self.parse_step = 2
            elif self.parse_step == 3:
                if tag == 'table':
                    if self.table_count >= 2:
                        self.parse_step = 4
                    self.table_count += 1
            elif self.parse_step == 4:
                if tag == 'tr':
                    self.parse_step = 5
                    self.quantity = ''
            elif self.parse_step == 5:
                if tag == 'td':
                    self.parse_step = 6
                    self.str_work = ''
            elif self.parse_step == 6:
                if self.tag_now == 'input':
                    attrs = dict(attrs)
                    if 'name' in attrs and 'type' in attrs and attrs['type'] == 'text':
                        self.quantity = attrs['name']
                        
    def handle_data(self, data):
        if self.parse_step >= 0:
            if self.b_start:
                if self.parse_step == 0:
                    if self.tag_now == 'title':
                        if u'注文入力' in data.decode('utf-8') and u'信用返済' in data.decode('utf-8'):
                            self.parse_step = 1
                            self.b_result = True
                elif self.parse_step == 2:
                    if self.tag_now == 'br':
                        if u'一括指定' in data.decode('utf-8'):
                            self.parse_step = 3
                elif self.parse_step == 6:
                    if not self.str_work:
                        self.str_work = data
            self.b_start = False
            
    def handle_endtag(self, tag):
        if self.parse_step >= 0:
            if self.parse_step == 4:
                if tag == 'table':
                    self.parse_step = -1
                    # データ取得
                    if len(self.tr_data) >= 2:
                        tr_inx = 0
                        for td_data in self.tr_data:
                            if tr_inx > 0:
                                self.tatedama.append((td_data[searchTdDataTag(self.tr_data[0], '建日')],
                                                     float(td_data[searchTdDataTag(self.tr_data[0], '建単価')]),
                                                     int(deleteComma(td_data[searchTdDataTag(self.tr_data[0], '建株数')]).decode('utf-8').replace(u'株', '')),
                                                      td_data[searchTdDataTag(self.tr_data[0], '数量')]))
                            tr_inx += 1
            elif self.parse_step == 5:
                if tag == 'tr':
                    self.parse_step = 4
                    if not self.tr_data:
                        self.tr_data.append(self.td_data[:])
                        del self.td_data[:]
                        return
                    else:
                        qa_inx = searchTdDataTag(self.tr_data[0], '数量')
                        if qa_inx >= 0:
                            self.td_data[qa_inx] = self.quantity
                            if len(self.td_data) == len(self.tr_data[0]):
                                self.tr_data.append(self.td_data[:])
                                del self.td_data[:]
                                return
                    # error発生
                    self.b_result = False
                    self.parse_step = -1
            elif self.parse_step == 6:
                if tag == 'td':
                    self.parse_step = 5
                    self.td_data.append(self.str_work)
                    

#   @brief  建玉リストを切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 buyHOrderEntryTateList.doのresponse(html/utf-8)
#   @return (結果, code, caID, list(建日, 単価, 株数, frame名))
def responseRepLeverageStockOrderTateList(html_u8):

    parser = RepOrderTateListParser()
    parser.feed(html_u8)
    parser.close()
    
    return (parser.b_result,
            parser.code,
            parser.caIQ,
            parser.tatedama)


#   @brief  [Debug]Shift-JISで送られてきたhtmlをUTF8に変換してからファイル出力する
#   @param  html_sjis   response(html/Shift-JIS)
def debugOutputShiftJisHTMLToFile(html_sjis, filename):

    f = open(filename, 'w')
    f.write(html_sjis.decode('cp932').encode('utf-8'))
    f.close()

#   @brief  [Debug]UTF8で送られてきたhtmlをそのままファイル出力する
#   @param  html_u8     response(html/UTF-8)
def debugOutputHTMLToFile(html_u8, filename):

    f = open(filename, 'w')
    f.write(html_u8)
    f.close()

'''
if __name__ == "__main__":
 
    url = "https://k.sbisec.co.jp/bsite/visitor/loginUserCheck.do"
    request = urllib2.Request(url)
    request.add_header('Accept', 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8');
    request.add_header('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');
    request.add_header('User-Agent', 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36');

    response = urllib2.urlopen(request)
    
    print responseLoginMobile(response.read())
    
    response.close()
    
if __name__ == "__main__":
 
    strbuff = open('mobile_login_ok.html').read()
    print responseLoginMobile(strbuff)

if __name__ == "__main__":

    strbuff = open('test.html').read()
    for elem in getPortfolioMobile(strbuff):
        print elem

if __name__ == "__main__":

    strbuff = raw_input('>> ')
    print responseGetEntranceOfPortfolioTransmission(strbuff)

if __name__ == "__main__":

    strbuff = open('portfolio.html').read()
    print getPortfolioPC_Owned(strbuff)

if __name__ == "__main__":

    strbuff = open('order_list4.html').read()
    ret = getTodayExecInfo(strbuff)
    print ret[0]
    for w in ret[1]:
        print w

if __name__ == "__main__":

    strbuff = open('buy_order_ex.html').read()
    print responseStockOrderExec(strbuff)

if __name__ == "__main__":

    strbuff = open('buy_order_entry.html').read()
    print getStockOrderRegistID(strbuff, 1)

''' 
