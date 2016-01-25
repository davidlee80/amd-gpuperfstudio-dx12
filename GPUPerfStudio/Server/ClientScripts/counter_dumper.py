import urllib2
import StringIO
import time

url = 'http://127.0.0.1/api/fa/Profiler.xml?Stream=1500'

f = urllib2.urlopen( url )


def GetFrame():
    dic = {}
    buffer = f.readline()
    while 1:
        buffer = f.readline()
        buffer = buffer.rstrip("\r\n")
        fi = buffer.split(": ")
        if len(fi)!=2: 
            break
        dic[fi[0]]=fi[1]

    size = int( dic["Content-Length"] )
    data = f.read(size)
    buffer = f.readline()
    print size
    return data



while 1:

    init = time.clock()
    data = GetFrame()
    #print data
    print time.clock() - init