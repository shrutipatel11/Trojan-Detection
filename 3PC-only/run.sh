taskgraph="mbench"
n_3pc=3
ip_3pc_scheduler="127.0.0.1"
ip_3pc_slaves=("127.0.0.1" "127.0.0.1" "127.0.0.1")
port_3pc_slaves=(10001 10002 10003 10004 10005)
task_3pc_slaves=(30001 30002 30003 30004 30005)
tpc_core=(1 2 3)
output="Report_3pc_only"

taskset -c 0 python 3pc_scheduler.py $taskgraph $n_3pc "${ip_3pc_slaves[@]:0:$n_3pc}" "${port_3pc_slaves[@]:0:$n_3pc}" $output &
sleep 2

for (( i=0; i<$n_3pc; i++ ))
do
  taskset -c "${tpc_core[i]}" python 3pc_slave.py $ip_3pc_scheduler "${port_3pc_slaves[i]}" "${task_3pc_slaves[$i]}" &
done
