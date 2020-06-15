import os
import ast
from multiprocessing import Pool, Process, Manager
from subprocess import Popen, PIPE
import socket
import select
import time
import sys

m1 = Manager()
processes = m1.list()
cores = m1.list()
tpc_results = m1.dict()
report = m1.dict()

tasks = {}
edges = []

argc = 1
taskgraph = sys.argv[argc]
argc+=1
ncores = int(sys.argv[argc])
argc+=1
ips = []
for i in range(ncores):
    ips.append(sys.argv[argc])
    argc+=1
port= []
for i in range(ncores):
    port.append(int(sys.argv[argc]))
    argc+=1
output = sys.argv[argc]

s = [socket.socket() for _ in range(ncores)]
for sckt in s:
    sckt.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
client = [0]*len(s)
addr = [0]*len(s)

for i in range(len(s)):
    s[i].bind((ips[i], port[i]))
    print "Socket",i,"binded to",port[i]
    s[i].listen(5)
    print "Socket",i,"is listening"

i=0
count = 0
timeout = 0.1
while True:
    ready_sockets, _, _ = select.select([s[i]], [], [], timeout)
    if ready_sockets:
        cl,ad = s[i].accept()
        client[i] = cl
        addr[i] = ad
        print "Got connection from",addr[i]
        count+=1
        if count==len(s):
            break
    else:
        i=(i+1)%(len(s))

#Find children of a node
def find_children(edge_list, parent):
    children = []
    for element in edge_list:
        if element[0] == parent:
            children.append(element[1])
    return children

#Find parents of a node
def find_parents(edge_list, child):
    parents = []
    for element in edge_list:
        if element[1] == child:
            parents.append(element[0])
    return parents

# Read the task graph
def read_task_graph(report):
    f = open(taskgraph,"r")
    if f.mode=="r":
        contents = f.readlines()

        #store the graph nodes(tasks) and dependencies(edges) in the lists
        for x in contents:
            if x.find("TASK")!=-1:
                start_index = 5
                while True:
                    if x[start_index]==' ':
                        break
                    start_index+=1
                task_name = x[5:start_index]

                start_index2 = start_index+4
                while True:
                    if x[start_index2]=='\n':
                        break
                    start_index2+=1
                id = x[start_index+4:start_index2]
                tasks[int(id)]=task_name  #add the task to the task list
                report[int(id)]=[0,0,0]

            elif x.find("ARC")!=-1:
                tempindex = x.find("FROM")+5
                start_index = tempindex
                while True:
                    if x[start_index]==' ':
                        break
                    start_index+=1
                t1 = x[tempindex:start_index]          #first task of the edge

                start_index2 = start_index+3
                while True:
                    if x[start_index2]=='\n':
                        break
                    start_index2+=1
                t2 = x[start_index+3:start_index2]      #second task of the edge

                edges.append([int(t1),int(t2)])    #add the edge to the edge list


def schedule_process(processes, tpc_results, cores, edges, start_time, report):
    i=0
    cpu = 0
    while True:
        #Find an idle core to schedule the process
        while True:
            if cores[i]==0:
                cpu = i
                break
            else:
                i = (i+1)%ncores

        if len(processes)==0:
            continue
        tid = processes[0]
        tname = tasks[tid]
        processes.remove(tid)
        cores[cpu]=1

        cmdarg = str(tid) + ' ' + tname + ' '
        par = find_parents(edges,tid)
        lp = len(par)
        for k in range(lp):
            addresult = tpc_results[par[k]][:-1]
            cmdarg += addresult
            if k!=lp-1:
                cmdarg+=','
        print tid,tname,"scheduled in cpu",cpu
        cmdarg+='A'
        # print "start time",time.time()-start_time

        ans = round(time.time()-start_time,2)
        get = report[tid]
        report[tid] = [str(cpu), str(ans), 0]

        if tname=='src':
            cmdarg+=' A'    #Delimiter

        l = 0
        print "len",len(cmdarg),cmdarg.index('A')
        while l<len(cmdarg):
            client[cpu].send(cmdarg[l:l+60000])
            l = l + 60000

        if tname=='sink':
            return

def receive_results(processes, tpc_results, cores, edges, start_time, report):
    timeout = 0.1  # in seconds
    j=0
    while True:
        ready_sockets, _, _ = select.select([client[j]], [], [], timeout)
        if ready_sockets:
            data = ''
            while True:
                ready, _, _ = select.select([client[j]], [], [], timeout)
                if ready:
                    rec = client[j].recv(60000)
                    data += rec
                    if 'A' in rec:
                        break

            id,name,result = data.split(" ")
            # print "end time ",name,time.time()-start_time
            id = int(id)
            ans = round(time.time()-start_time,2)
            get = report[id]
            report[id] = [get[0], get[1], str(ans)]

            if  name == 'sink':
                print len(result[:-1])
                return
            tpc_results[id] = result
            cores[j]=0
            c = find_children(edges,id)
            ancestors_executed = 0
            for child in c:
                par = find_parents(edges,child)
                for parent in par:
                    if parent not in tpc_results:
                        ancestors_executed = 1
                        break
                if ancestors_executed == 0:
                    processes.append(child)
        else:
            j=(j+1)%ncores


# ---------------------------Main program--------------------------------------
if __name__ == '__main__':
    start_time = time.time()
    read_task_graph(report)
    processes.append(1)
    for _ in range(ncores):
        cores.append(0)
    p1 = Process(target = schedule_process, args=[processes, tpc_results, cores, edges, start_time, report])
    p2 = Process(target = receive_results, args=[processes, tpc_results, cores, edges, start_time, report])
    p1.start()
    p2.start()
    p1.join()
    p2.join()
    print "Time taken : ",time.time()-start_time,"seconds"

    f= open(output,"w+")
    for key in report:
        s = tasks[key]+'\t'+report[key][0]+'\t'+report[key][1]+'\t'+report[key][2]+'\n'
        f.write(s)
    f.close()
