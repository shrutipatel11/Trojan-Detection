taskgraph="bench"
n_3pc=1
n_hgc=2
ip_3pc_scheduler="127.0.0.1"
ip_hgc_scheduler="127.0.0.1"
ip_3pc_slaves=("127.0.0.1" "127.0.0.1" "127.0.0.1")
ip_hgc_slaves=("127.0.0.1" "127.0.0.1" "127.0.0.1")
port_3pc_hgc=50011
port_3pc_slaves=(10001 10002 10003 10004 10005)
port_hgc_slaves=(20001 20002 20003 20004 20005)
task_3pc_slaves=(30001 30002 30003 30004 30005)
task_hgc_slaves=(40001 40002 40003 40004 40005)
tpc_core=(0 1)
hgc_core=(1 2 3)
output_3pc="Report_3pc"
output_hgc="Report_hgc"

taskset -c 0 python 3pcsched.py $taskgraph $n_3pc $ip_hgc_scheduler "${ip_3pc_slaves[@]:0:$n_3pc}" $port_3pc_hgc "${port_3pc_slaves[@]:0:$n_3pc}" $output_3pc&
sleep 2
taskset -c 1 python hgc_scheduler.py $taskgraph $n_hgc $ip_hgc_scheduler "${ip_hgc_slaves[@]:0:$n_hgc}" $port_3pc_hgc "${port_hgc_slaves[@]:0:$n_hgc}" $output_hgc&
sleep 3

for (( i=0; i<$n_hgc; i++ ))
do
  taskset -c "${hgc_core[i]}" python hgc_slave.py $ip_hgc_scheduler "${port_hgc_slaves[i]}" "${task_hgc_slaves[i]}" &
  sleep 4
done

for (( i=0; i<$n_3pc; i++ ))
do
  taskset -c "${tpc_core[i]}" python 3pc_slave.py $ip_3pc_scheduler "${port_3pc_slaves[i]}" "${task_3pc_slaves[$i]}" &
  sleep 4
done
