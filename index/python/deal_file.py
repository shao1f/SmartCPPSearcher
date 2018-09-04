#!/usr/bin/env python
# coding=utf-8

import os
import sys
import re
from bs4 import BeautifulSoup
reload(sys)
sys.setdefaultencoding('utf8')

input_path = "../data/boost"
output_path = "../data/output/line_file"
html_prefix = 'https://www.boost.org/doc/libs/1_68_0/' 

################摘抄自CSDN###############
##过滤HTML中的标签
#将HTML中标签等信息去掉
#@param htmlstr HTML字符串.
def filter_tags(htmlstr):
    #先过滤CDATA
    re_cdata=re.compile('//<!\[CDATA\[[^>]*//\]\]>',re.I) #匹配CDATA
    re_script=re.compile('<\s*script[^>]*>[^<]*<\s*/\s*script\s*>',re.I)#Script
    re_style=re.compile('<\s*style[^>]*>[^<]*<\s*/\s*style\s*>',re.I)#style
    re_br=re.compile('<br\s*?/?>')#处理换行
    re_h=re.compile('</?\w+[^>]*>')#HTML标签
    re_comment=re.compile('<!--[^>]*-->')#HTML注释
    s=re_cdata.sub('',htmlstr)#去掉CDATA
    s=re_script.sub('',s) #去掉SCRIPT
    s=re_style.sub('',s)#去掉style
    s=re_br.sub(' ',s)#将br替换为空格
    s=re_h.sub('',s) #去掉HTML 标签
    s=re_comment.sub('',s)#去掉HTML注释
    #去掉多余的空行
    blank_line=re.compile('\n+')
    #将多个空格合并为一个
    s=blank_line.sub(' ',s)
    s=replaceCharEntity(s)#替换实体
    return s
 
##替换常用HTML字符实体.
#使用正常的字符替换HTML中特殊的字符实体.
#你可以添加新的实体字符到CHAR_ENTITIES中,处理更多HTML字符实体.
#@param htmlstr HTML字符串.
def replaceCharEntity(htmlstr):
    CHAR_ENTITIES={'nbsp':' ','160':' ',
                'lt':'<','60':'<',
                'gt':'>','62':'>',
                'amp':'&','38':'&',
                'quot':'"','34':'"',}
    
    re_charEntity=re.compile(r'&#?(?P<name>\w+);')
    sz=re_charEntity.search(htmlstr)
    while sz:
        entity=sz.group()#entity全称，如>
        key=sz.group('name')#去除&;后entity,如>为gt
        try:
            htmlstr=re_charEntity.sub(CHAR_ENTITIES[key],htmlstr,1)
            sz=re_charEntity.search(htmlstr)
        except KeyError:
            #以空串代替
            htmlstr=re_charEntity.sub('',htmlstr,1)
            sz=re_charEntity.search(htmlstr)
    return htmlstr
 

def get_FileList(input_path):
    """
    获得文件列表,只需要html文件
    """
    file_list = []
    for root,dirs,files in os.walk(input_path):
        for name in files:
            if name[-5:] == '.html':
                file_list.append(root + '/' + name)
    return file_list

def parse_url(file):
    """
    解析html网址
    """
    return html_prefix + file[14:]

def parse_title(file):
    """
    解析html的标题
    """
    soup = BeautifulSoup(file,'html.parser')
    title = soup.find('title')
    if title:
        return title.text

def parse_content(file):
    """
    解析html的正文
    """
    return filter_tags(file)
    
def parse_file(file_list):
    file = open(file_list).read()
    return parse_url(file_list),parse_title(file),parse_content(file)

def final_deal():
    """
    做最终的文件处理,将url,title,content写成一行到output下的line_file中
    """
    file_lists = get_FileList(input_path)
    # 打开output下的文件进行处理
    with open(output_path,'w') as f:
        for i in file_lists:
            res = parse_file(i)
            # 有的时候title可能是空的,这种没有标题的可以认为不是主要内容,直接丢弃
            if res[0] and res[1] and res[2]:
                try:
                    f.write(res[0] + '\2' + res[1] + '\2' + res[2] + '\n') # 以换行结束
                except:
                    err_f = open('./err.log','a')
                    err_f.write(i + '\t' + res[0] + '\n')
                    err_f.close()


if __name__ == '__main__':
    final_deal()

def test():
    file_lists = get_FileList(input_path)
    # print file_lists[26],parse_url(file_lists[26])
    for i in file_lists:
        res = parse_file(i)
        print res[0];
        print res[1];
        print res[2];

    # res = open(file_lists[26]).read()
    # soup = BeautifulSoup(res,'html.parser')
    # tit = soup.find('title').text
    # _str = filter_tags(res);
    # print parse_content(res)
# test()

