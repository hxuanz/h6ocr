# encode: utf-8
import requests
import base64
import sys
import time
import json

def h6ocr(file_name):
    iamge =  open(file_name, 'rb').read()
    obj = '{"scale":[{"id": "s1-q1-o1","text": "r_GT", "value": "" },{"id": "s1-q1-o1","text": "A/G", "value": "" },{"id": "s1-q1-o1","text": "ALB", "value": "" },{"id": "s1-q1-o1","text": "ALP", "value": "" },{"id": "s1-q1-o1","text": "ALT", "value": "" },{"id": "s1-q1-o1","text": "ASL", "value": "" },{"id": "s1-q1-o1","text": "CHO", "value": "" },{"id": "s1-q1-o1","text": "Crea", "value":""},{"id": "s1-q1-o1","text": "DBILI", "value": "" }]}'
    r = requests.post("http://127.0.0.1:12345/ocr/xhd", data={"imageData":base64.b64encode(iamge), "scaleData":obj})
    if r.status_code == 200:
        print(r.text)

def h6test2():
    s = """
    WBC
    RBC
    HGB
    HCT
    MCV
    MCH
    MCHC
    PLT
    LYMPH%
    NEUT%
    MONO%
    EO%
    BASO%
    LYMPH#
    NEUT#
    MONO#
    EO#
    BASO#
    RDW-CV
    PDW
    MPV
    PCT
    XHDB
    CRP
    """
    obj =json.dumps({"scale":[{"id": "s1-q1-o1","text": text.strip(), "value": ""} for text in s.split('\n') if text.strip()]})
    print(obj) #{"scale": [{"text": "WBC", "id": "s1-q1-o1", "value": ""}, {"text": "RBC", "id": "s1-q1-o1", "value": ""}, {"text": "HGB", "id": "s1-q1-o1", "value": ""}, {"text": "HCT", "id": "s1-q1-o1", "value": ""}, {"text": "MCV", "id": "s1-q1-o1", "value": ""}, {"text": "MCH", "id": "s1-q1-o1", "value": ""}, {"text": "MCHC", "id": "s1-q1-o1", "value": ""}, {"text": "PLT", "id": "s1-q1-o1", "value": ""}, {"text": "LYMPH%", "id": "s1-q1-o1", "value": ""}, {"text": "NEUT%", "id": "s1-q1-o1", "value": ""}, {"text": "MONO%", "id": "s1-q1-o1", "value": ""}, {"text": "EO%", "id": "s1-q1-o1", "value": ""}, {"text": "BASO%", "id": "s1-q1-o1", "value": ""}, {"text": "LYMPH#", "id": "s1-q1-o1", "value": ""}, {"text": "NEUT#", "id": "s1-q1-o1", "value": ""}, {"text": "MONO#", "id": "s1-q1-o1", "value": ""}, {"text": "EO#", "id": "s1-q1-o1", "value": ""}, {"text": "BASO#", "id": "s1-q1-o1", "value": ""}, {"text": "RDW-CV", "id": "s1-q1-o1", "value": ""}, {"text": "PDW", "id": "s1-q1-o1", "value": ""}, {"text": "MPV", "id": "s1-q1-o1", "value": ""}, {"text": "PCT", "id": "s1-q1-o1", "value": ""}, {"text": "XHDB", "id": "s1-q1-o1", "value": ""}, {"text": "CRP", "id": "s1-q1-o1", "value": ""}]}
    iamge = open('d:/ocr/11.jpg', 'rb').read()
    r = requests.post("http://127.0.0.1:12345/ocr/xhd", data={"imageData": base64.b64encode(iamge), "scaleData": obj})
    if r.status_code == 200:
        print(r.text)

if __name__ == '__main__':
    file_name = "d:/ocr/1.jpg"
    if len(sys.argv) == 2:
        file_name = sys.argv[1]

    t1 = time.time()
    h6ocr(file_name)
    #h6test2()
    t2 = time.time()
    print(t2 - t1)

