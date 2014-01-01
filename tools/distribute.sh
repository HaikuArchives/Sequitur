distribute ()
{
	if [ $# -lt 3 ]; then
		echo "Usage: distribute rootdir destdir project-name" 1>&2
		return
	fi

	typeset _rootdir
	typeset _destdir
	typeset _pubspec
	
	typeset _PUBLICFILELIST
	typeset _INCLUDEFILELIST
	typeset _numsec
	typeset _numfile
	typeset _filedir
	typeset _filelist
	typeset OLDIFS
	typeset LINE
	typeset i
	
	_rootdir=$1
	_destdir=$2
	_pubspec="(Public-$3)"
	
	_PUBLICFILELIST=/tmp/$3.dist.$$
	_INCLUDEFILELIST=/tmp/$3.incl.$$
	
	pushd .
	cd $_rootdir
	
	# Make list of file sections
	find . -name "$_pubspec" -print > "$_PUBLICFILELIST"
	
	_numsec=$(wc -l "$_PUBLICFILELIST" | awk '{print $1'})
	
	echo "Found $_numsec public sections."
	
	echo "Determining all files to include in distribution \"$3\"..."
	
	# make sure the files exist and are empty
	rm -f "$_INCLUDEFILELIST"
	touch "$_INCLUDEFILELIST"
	
	OLDIFS=$IFS
	IFS=
	exec 3< "$_PUBLICFILELIST"
	while read LINE 0<&3
	do
		if [ -f "$LINE" ]; then
			IFS=$OLDIFS
			_filedir=$(dirname "$LINE")
			_filelist=$(cat "$LINE")
			if [ "$_filelist" = "" -o "$_filelist" = "*" ]; then
				for i in ${_filedir}/*; do
					if [ -f "$i" ]; then
						echo >> "$_INCLUDEFILELIST" "$i"
					fi
				done
				# echo >> "$_INCLUDEFILELIST" "$filedir"
			else
				echo >> "$_INCLUDEFILELIST" "$LINE"
				for i in $_filelist; do
					echo >> "$_INCLUDEFILELIST" "$_filedir/$i"
				done
			fi
			IFS=
		else
			echo "*** Warning: $LINE is not a file."
		fi
	done
	exec 3<&-
	IFS=$OLDIFS
	
	_numfile=$(wc -l "$_INCLUDEFILELIST" | awk '{print $1'})
	
	echo "Found $_numfile files and directories to include."
	
	OLDIFS=$IFS
	IFS=
	exec 3< "$_INCLUDEFILELIST"
	while read LINE 0<&3
	do
		echo "Copying $LINE"
		mkdir --parents $(dirname "$_destdir/$LINE")
		copyattr --data "$LINE" "$_destdir/$LINE"
	done
	exec 3<&-
	IFS=$OLDIFS
	
	popd
	
	rm "$_INCLUDEFILELIST" "$_PUBLICFILELIST"
	
    return 0
}
