import urllib2
import re

# input:
#    address: the address of a machine running PerfStudio Server
#    frames:  number of frames to capture
#
# Connects to the specified URL an returns a list of api calls in the following format:
# <interface> <function name> <parameter list> <returned value>
# the last parameter is omited if the funtion returns null
#
def GetLoggedData( address, frames ):

    f = urllib2.urlopen( 'http://%s/api/fa/FullAPITrace.txt?Stream=100000' % address )
    database = []
    
    print "Capturing Frames"
    LoggedData = ""
    for i in range(0,frames):
        
        print " Frame",
        print i,
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
        LoggedData = LoggedData + f.read( size )
        buffer = f.readline()
        print "OK"

    f.close()    

    
    LoggedData = LoggedData .rstrip('\n')
    lines = LoggedData.split("\n")
    
    print "Parsing log and creating database ",

    for i in lines:
        m = re.match( "(\\w+)_(\\w+)\\( (.*?) \\) *=* *(\\w+)*", i )
        
        l = [ m.group(1), m.group(2), m.group(3).split(", ") ] 

        if m.lastindex == 4:
            l.append( m.group(4) )
            
            
        database.append( l )
        
    print "Done"

    return database


#
#
#   Prints the whole API database 
#
#
def PrintAPILog( database ):

    histo = {}

    for call in database:



        print call[0] + "::" + call[1] + "(" ,


        for param in call[2]:
            print param,


        print ")",
        
        if len(call) == 4:
            print "= " + param[3],
            
        print 
        




def DisplayHistogram( histo ):
    items = [(v, k) for k, v in histo.items()]
    items.sort()
    items.reverse()      
      
    for v, k in items:
        print "%3s %s" % (v,k)



#
#
#  API usage count
#
#
def APIHistogram( database ):

    histo = {}

    for call in database:

        str = call[0] + "::" + call[1]

        if histo.has_key( str ):
            histo[ str ] = histo[ str ] + 1
        else:
            histo[ str ] =  1
        
    DisplayHistogram( histo )


#
#
#  Shader usage count
#
#
def ShaderHistogram( database ):

    histo = {}

    for call in database:

        if ( call[1] == "VSSetShader" or call[1] == "GSSetShader" or call[1] == "PSSetShader" ):

            shader_pointer = call[2][1]
            
            if histo.has_key( shader_pointer ):
                histo[ shader_pointer ] = histo[ shader_pointer ] + 1
            else:
                histo[ shader_pointer ] =  1
        
    DisplayHistogram( histo )




#   
# connect to local host and take 5 frames
#
database = GetLoggedData( "127.0.0.1", 5 )

"""
print
print "API Log"
print "-------"
PrintAPILog( database )
"""

print
print "API Usage Histogram"
print "-------------------"
APIHistogram( database )

print
print "Shader Usage Histogram"
print "----------------------"
ShaderHistogram( database )

