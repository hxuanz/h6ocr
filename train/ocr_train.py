# -*- coding: utf-8 -*-
import subprocess
import os
import sys

FONT = "Times New Roman,"

lang = 'xhd'
fontname = 'time_new_roman'

PATH = 'D:/ocr/train'
text_files = [file for file in os.listdir(PATH) if file[-4:] == '.txt']

names = ["%s.%s.exp%d" % (lang, fontname, i) for i in range(len(text_files))]

commonds = ["cd %s" % PATH]
commonds.extend(['text2image --text=%s --outputbase=%s --font="%s" --fonts_dir="C:\Windows\Fonts"' % (text, names[i], FONT)
                      for i, text in enumerate(text_files)])

#对每一对训练图片和box文件，运行下面命令行
commonds.extend(['tesseract %s.tif %s box.train' % (name, name) for name in names])  #-l eng -psm 7 nobatch
#
commonds.append('unicharset_extractor ' + ' '.join( '%s.box' % name for name in names))
commonds.append('echo %fontname% 0 0 0 0 0 >font_properties')

commonds.append(('shapeclustering -F font_properties -U unicharset -O %s.unicharset' % lang) + ' '.join( '%s.tr' % name for name in names))
commonds.append(('mftraining -F font_properties -U unicharset -O %s.unicharset' % lang) + ' '.join( '%s.tr' % name for name in names))
commonds.append('cntraining ' + ' '.join( '%s.tr' % name for name in names))

commonds.append('rename normproto %s.normproto' % lang)
commonds.append('rename inttemp %s.inttemp' % lang)
commonds.append('rename pffmtable %s.pffmtable ' % lang)
commonds.append('rename unicharset %s.unicharset' % lang)
commonds.append('rename shapetable %s.shapetable' % lang)

commonds.append('combine_tessdata %s.'% lang)

commonds.append('del font_properties')
commonds.append('del %s.unicharset '% lang)
commonds.append('del %s.shapetable '% lang)
commonds.append('del %s.pffmtable '% lang)
commonds.append('del %s.inttemp '% lang)
commonds.append('del %s.normproto '% lang)
commonds.append('del *.tr')
commonds.append('del *.tif')
commonds.append('del *.box')

commond = '\n'.join(commonds)
print(commond)

open('%s/run.bat' % PATH, 'w').write(commond)
#retcode = subprocess.call(commond, shell=True)
# subprocess.Popen("gedit abc.txt", shell=True)