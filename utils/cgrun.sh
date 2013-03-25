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
cgexec -g $clist:$name --sticky su -c "cd $dir && $command" - $targetuser
echo "======  END  ======"
echo

echo -n "cleanup cgroup..."
cgdelete -g $clist:$name; test ok

