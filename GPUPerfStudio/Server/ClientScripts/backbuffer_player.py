import Tkinter as tk
from PIL import Image, ImageTk
import urllib2
import StringIO

root = tk.Tk()
cv1 = tk.Canvas(root, width=500, height=500)
cv1.pack(fill='both', expand='yes')

url = 'http://127.0.0.1/sc/pic.bmp?Stream=15000'
#url = 'http://155.31.20.200/axis-cgi/mjpg/video.cgi?resolution=320x240'

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
    data = GetFrame()	

    image1 = Image.open(StringIO.StringIO(data))
    tkimage1 = ImageTk.PhotoImage(image1)
    cv1.delete(tk.ALL)
    id = cv1.create_image(0, 0, image=tkimage1, anchor=tk.NW)

    root.update_idletasks()
    root.update()
