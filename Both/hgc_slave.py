import os
import ast
from multiprocessing import Pool, Process, Manager
from subprocess import Popen, PIPE
import socket
import select
import time
import sys

ip_scheduler = sys.argv[1]
port = int(sys.argv[2])
tport = int(sys.argv[3])

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.connect((ip_scheduler, port))

tsock = socket.socket()
tsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
ip = '127.0.0.1'
tsock.bind(('127.0.0.1', tport))

def run_process(task,ip,port):
    os.system('./{} {} {}'.format(task,ip,port))

def connect(s,tsock,args,task,id):
    tsock.listen(5)
    cli, addr = tsock.accept()
    # print 'Got connection from', addr
    if len(args)>0:
        args.replace('A','')
        if len(args)>1:
            inplen = str(len(list(args.split(","))))
            cli.send(inplen)
            time.sleep(1.5)
            i = 0
            while i<len(args):
                cli.send(args[i:i+60000])
                i = i + 60000
            time.sleep(2)
            cli.send('A')

    # time.sleep(1)
    result = id+ ' '+task+' '
    temp = ''
    while True:
        timeout = 0.1  # in seconds
        ready_sockets, _, _ = select.select([cli], [], [], timeout)
        if ready_sockets:
            rec = cli.recv(60000)
            temp += rec
            if 'A' in rec:
                result += temp
                # print "Received from the client : ", len(result)
                tsock.close()
                time.sleep(1)
                # print "Sending result"
                i = 0
                while i<len(result):
                    s.send(result[i:i+60000])
                    i = i + 60000
                break


while True:
    timeout = 0.1  # in seconds
    cmdarg = ''
    while True:
        ready_sockets, _, _ = select.select([s], [], [], timeout)
        if ready_sockets:
            argl = s.recv(60000)
            if len(argl)>0:
                # print "Packet length received ",len(argl)
                cmdarg+=argl
                if 'A' in argl:
                    # print "len received",len(cmdarg)
                    break
    time.sleep(2)
    tosend = list(cmdarg.split(" "))

    id = tosend[0]
    task = tosend[1]
    args = []
    if len(tosend)>1:
        id = tosend[0]
        task = tosend[1]
        args = tosend[2]
    # print id,task,len(args)
    p2 = Process(target = run_process, args=[task,ip,tport])
    p1 = Process(target = connect, args = [s,tsock,args,task,id])
    p1.start()
    p2.start()
    p1.join()
    p2.join()
