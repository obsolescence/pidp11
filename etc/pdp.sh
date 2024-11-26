procs=`sudo screen -ls pidp11 | egrep '[0-9]+\.pidp11' | wc -l`
echo $procs
if [ $procs -ne 0 ]; then
	sudo screen -d -r
fi
