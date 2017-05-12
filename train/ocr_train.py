# -*- coding: utf-8 -*-
import subprocess
import os
import sys

FONT = "Times New Roman,"
fontname = 'time_new_roman'

def build_commond(lang):
    text_files = [file_name for file_name in os.listdir('train_text') if file_name[:len(lang)] == lang]

    names = ["%s.%s.exp%d" % (lang, fontname, i) for i in range(len(text_files))]
    commonds = []
    # 生成训练集  -- 创建tif/box文件  [box文件名和tif文件名要相同，并且在相同路径下]
    commonds.extend(['text2image --text=train_text/%s --outputbase=%s --font="%s" --fonts_dir="C:\Windows\Fonts"' % (text, names[i], FONT)
                          for i, text in enumerate(text_files)])

    #对每一对训练图片和box文件，运行下面
    commonds.extend(['tesseract %s.tif %s box.train' % (name, name) for name in names])  #-l eng -psm 7 nobatch
    #unicharset_ex程序处理box文件, 生成unicharset数据文件
    commonds.append('unicharset_extractor ' + ' '.join( '%s.box' % name for name in names))
    # 提供字体信息
    commonds.append('echo %fontname% 0 0 0 0 0 >font_properties')

    # 聚类
    commonds.append(('shapeclustering -F font_properties -U unicharset -O %s.unicharset' % lang) + ' '.join( '%s.tr' % name for name in names))
    commonds.append(('mftraining -F font_properties -U unicharset -O %s.unicharset' % lang) + ' '.join( '%s.tr' % name for name in names))
    commonds.append('cntraining ' + ' '.join( '%s.tr' % name for name in names))

    commonds.append('rename normproto %s.normproto' % lang)
    commonds.append('rename inttemp %s.inttemp' % lang)
    commonds.append('rename pffmtable %s.pffmtable ' % lang)
    commonds.append('rename unicharset %s.unicharset' % lang)
    commonds.append('rename shapetable %s.shapetable' % lang)

    ##########  合并所有文件 #########
    commonds.append('combine_tessdata %s.' % lang)

    commonds.append('del font_properties')
    commonds.append('del %s.unicharset '% lang)
    commonds.append('del %s.shapetable '% lang)
    commonds.append('del %s.pffmtable '% lang)
    commonds.append('del %s.inttemp '% lang)
    commonds.append('del %s.normproto '% lang)
    commonds.append('del *.tr')
    commonds.append('del *.tif')
    commonds.append('del *.box')

    commonds.append('move %s.traineddata %%TESSDATA_PREFIX%%/tessdata/' % lang)
    #commonds.append('move %s.traineddata result/' % lang)

    commond = '\n'.join(commonds)
    print(commond)
    return commond

if __name__ == '__main__':
    lang = 'xhd_value'  #
    if len(sys.argv) == 2:
        lang = sys.argv[1]

    commond = build_commond(lang)
    open('%s_run.bat' % (lang), 'w').write(commond)

    #retcode = subprocess.call(commond, shell=True)
    # subprocess.Popen("gedit abc.txt", shell=True)