#!/bin/sh

# config
name="foo"

# stuff
clist="blkio,cpuset"
command="$@"
dir=$(pwd)
targetuser=$(logname)

# colors
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
NORMAL=$(tput sgr0)

# helpers
anywait() {
	for pid in "$@"; do
		while [ -z $(kill -0 "$pid" 2>/dev/null || echo failed) ]; do
			sleep 0.2
		done
	done
}

test() {
	if [ $? -eq 0 ];then
		if [ -n "$1" ];then
			printf "${GREEN}$1${NORMAL}\n"
		fi
	else
		printf "${RED}failed${NORMAL}\n"
		exit 1
	fi
}

echo -n "setup cgroup..."
cgcreate -g $clist:$name; test
cgset -r blkio.weight=100 $name; test
cgset -r cpuset.cpus=0-3 $name; test
cgset -r cpuset.mems=0 $name; test ok

echo
echo "====== START ======"
pidstore=/tmp/su.$$
cgexec -g $clist:$name --sticky su -c "cd $dir && $command 3>&- & echo \$! 1>&3" $targetuser 3>$pidstore
child=$(cat $pidstore)
trap "kill $child" INT
anywait $child
rm -f $pidstore
echo "======  END  ======"
echo

echo -n "cleanup cgroup..."
cgdelete -g $clist:$name; test ok

