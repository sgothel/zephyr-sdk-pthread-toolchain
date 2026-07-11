
set_target_triple() {
	bname=$1
	prefix=$2
	if [ -z "${bname}" -o -z "${prefix}" ] ; then
		echo "Usage <basename> <prefix>"
		exit 1
	fi
	local bname_triple=$(echo ${bname} | sed "s/${prefix}-//g")
	OIFS=$IFS
	IFS='-'
	read -r target_cpu target_os target_abi <<< ${bname_triple}
	IFS=$OIFS
	target_triple="${target_cpu}-${target_os}-${target_abi}"
	echo "Target triple ${target_triple}"
}

check_dir() {
	fname=$1
	if [ ! -d ${fname} ] ; then
		echo "Directory missing: ${fname}"
		exit 1;
	fi
}

check_file() {
	fname=$1
	if [ ! -f ${fname} ] ; then
		echo "File missing: ${fname}"
		exit 1;
	fi
}

move_to_old() {
	fname=$1
	if [ -e ${fname}.old ] ; then
		echo "Removing ${fname}.old"
		rm -rf ${fname}.old
	fi
	if [ -e ${fname} ] ; then
		echo "Move ${fname} to ${fname}.old"
		mv ${fname} ${fname}.old
	fi
}
