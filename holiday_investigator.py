# -*- coding: utf-8 -*- 
 
import urllib2

from HTMLParser import HTMLParser
 
#   @brief  今日が休日か調べるhtml-parser
#   @note   "https://yonelabo.com/today_holyday/"をあてにする
class TodayHolydayParser(HTMLParser):
 
    def __init__(self):
        HTMLParser.__init__(self)
        #
        self.tag_now = ''
        self.data_tag = ''
        self.on_connect_string = False
        self.div_string = u''
        #
        self.is_holyday = False     # 休日フラグ
        self.b_result = False       # 処理の成否フラグ
 
    def handle_starttag(self, tag, attrs): # 開始タグを扱うためのメソッド
        self.tag_now = tag
        self.data_tag = 'start'
    def handle_endtag(self, tag): # 終了タグを扱うためのメソッド
        self.tag_now = tag
        self.data_tag = 'end'
 
    def handle_data(self, data): # 要素内用を扱うためのメソッド
        if not self.b_result:
            # divから/divまでの文字列を繋ぎ、その有り様を調べる
            if self.tag_now == 'div' and self.data_tag == 'start':
                self.div_string = ''
                self.on_connect_string = True
            if self.on_connect_string:
                self.div_string += data.rstrip().decode('utf-8')
            if self.tag_now == 'div' and self.data_tag == 'end':
                self.on_connect_string = False
                if u'今日は祝日ではありません' in self.div_string:
                    self.is_holyday = False
                    self.b_result = True
                elif u"今日は休日(" in self.div_string and u")です" in self.div_string:
                    self.is_holyday = True
                    self.b_result = True
                elif u"今日は" in self.div_string and u"日(祝日)" in self.div_string:
                    self.is_holyday = True
                    self.b_result = True

#   @brief  今日が休日か調べる
#   @param  html_u8 html/utf-8
#   @return 成否 
def investigateHoliday(html_u8):

    parser = TodayHolydayParser()
    parser.feed(html_u8)
    parser.close()
    
    return (parser.b_result, parser.is_holyday)

'''
if __name__ == "__main__":
 
    url = "https://yonelabo.com/today_holyday/"
    request = urllib2.Request(url)
    request.add_header('Accept', 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8');
    request.add_header('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');
    request.add_header('User-Agent', 'Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36');

    response = urllib2.urlopen(request)
    print investigateHoliday(response.read().decode('utf-8'))
    response.close()
'''