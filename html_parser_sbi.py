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
    
#   @brief  C++のeOrderTypeカバーclass
class eOrderTypeEnum:

    def __init__(self):
       self.eOrderType = { '買':1, '売':2, '訂正':3, '取消':4 }
    
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


#   @brief  ポートフォリオを切り出しclass
#   @note   SBI-PC[メイン]サイト用
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

    
#   @brief  ポートフォリオを切り出す(SBI-PC[メイン]サイト用)
#   @param  html_sjis   ポートフォリオを含むresponse(html/Shift-JIS)
#   @return list[銘柄コード, 現値, 始値, 高値, 安値, 前日終値, 出来高]
def getPortfolioPC(html_sjis):

    #strbuff = open('portfolio_get.html').read()

    parser = PortfolioParserPC()
    parser.feed(html_sjis.decode('cp932').encode('utf-8'))
    parser.close()
    
    return parser.portfolio


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
    
    
#   @brief  regist_idを切り出すclass
#   @note   買注文入力画面(SBI-mobile[バックアップ]サイト)用
class StockOrderRegistIDParser(RegistIDParser):
 
    def __init__(self, title_str, order_type):
        RegistIDParser.__init__(self)
        self.title_str = title_str
        self.order_tag = eOrderTypeEnum().getOrderTag(order_type)

    def handle_data(self, data):
        if not self.b_ok and self.b_start and self.tag_now == 'title':
            if self.title_str in data.decode('utf-8'):
                if '買' == self.order_tag:
                # 括弧の全/半角に表記ゆれがある…
                    if u'買)' in data.decode('utf-8') or u'買）' in data.decode('utf-8'):
                        self.b_ok = True
                elif '売' == self.order_tag:
                    if u'売）' in data.decode('utf-8'):
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


#   @brief  新規売買注文結果切り出しclass
#   @note   注文受付画面(SBI-mobile[バックアップ]サイト)用
class FreshOrderExParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        self.tag_now = ''
        self.b_start = False
        self.str_work = ''
        self.td_data = []
        self.table_count = 0
        self.parse_step = 0 #parse段階 0:対象titleタグ走査
                            #          1:タイトルチェック、現物/信用フラグセット
                            #          2:対象brタグ走査
                            #          3:注文成否フラグセット
                            #          4:対象inputタグ走査、注文番号(内部値)取得
                            #          5:対象table走査
                            #          6:tdタグ走査 + tableタグを抜けたら5に戻す(テーブル2個分)
                            #          7:tdタグ終了までのデータ取得(空出ないものが見つかるまで)
                            #            +tdタグを抜けたら6に戻す
                            #         -1:完了
        self.b_result = False   #注文成否
        self.b_leverage = False #信用フラグ
        self.order_type = 0     #注文種別[eOrderType]
        self.order_id = -1      #注文番号
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
            if self.b_start:
                if self.parse_step == 1:
                    if self.tag_now == 'div' and u'注文受付' in data.decode('utf-8'):
                        self.parse_step = 2
                        if u'信用' in data.decode('utf-8'):
                            self.b_leverage = True
                        if u'買' in data.decode('utf-8'):
                            eot = eOrderTypeEnum()
                            self.order_type = eot.getOrderType('買')
                        elif u'売' in data.decode('utf-8'):
                            eot = eOrderTypeEnum()
                            self.order_type = eot.getOrderType('売')
                            
                elif self.parse_step == 3:
                    if self.tag_now == 'br':
                        if u'ご注文を受付いたしました' in data.decode('utf-8') or u'ご注文を受付致しました' in data.decode('utf-8'):
                            self.parse_step = 4
                            self.b_result = True
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
                        self.code = int(self.td_data[0])
                        if u'東証' in self.td_data[1].decode('utf-8'):
                            self.investments = 'TKY'
                        elif u'PTS' in self.td_data[1].decode('utf-8'):
                            self.investments = 'JNX'
                        self.numbers = int(self.td_data[3].decode('utf-8').replace(u'株', ''))
                        self.value = float(deleteComma(self.td_data[5].decode('utf-8').replace(u'円', '')))
            elif self.parse_step == 7:
                if tag == 'td':
                    self.parse_step = 6
                    self.td_data.append(self.str_work)

    
#   @brief  注文(買)結果を切り出す(SBI-mobile[バックアップ]サイト用)
#   @param  html_u8 buyOrderEx.doのresponse(html/utf-8)
#   @return (解析結果, 注文番号, 取引所コード(str), 銘柄コード(int), 株数, 価格, 信用フラグ)
def responseFreshOrderExec(html_u8):

    parser = FreshOrderExParser()
    parser.feed(html_u8)
    parser.close()
    
    return (parser.b_result,
            parser.order_id,
            parser.investments,
            parser.code,
            parser.numbers,
            parser.value,
            parser.b_leverage,
            parser.order_type)
    
    

#   @brief  [Debug]Shift-JISで送られてきたhtmlをUTF8に変換してからファイル出力する
#   @param  html_sjis   response(html/Shift-JIS)
def debugOutputShiftJisHTMLToFile(html_sjis):

    f = open('logintest.txt', 'w')
    f.write(html_sjis.decode('cp932').encode('utf-8'))
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
    print getPortfolioRegistID(strbuff)

if __name__ == "__main__":

    strbuff = open('test.html').read()
    for elem in getPortfolioMobile(strbuff):
        print elem

if __name__ == "__main__":

    strbuff = raw_input('>> ')
    print responseGetEntranceOfPortfolioTransmission(strbuff)

if __name__ == "__main__":

    strbuff = open('buy_order_ex.html').read()
    print responseFreshOrderExec(strbuff)

if __name__ == "__main__":

    strbuff = open('buy_order_entry.html').read()
    print getStockOrderRegistID(strbuff, 1)
''' 
