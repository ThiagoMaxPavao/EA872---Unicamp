#!/bin/bash

usage(){
	echo "Usage: $0 filename"
	exit 1
}
 
is_file_exits(){
	local f="$1"
	[[ -f "$f" ]] && return 0 || return 1
}
[[ $# -eq 0 ]] && usage
 
if ( is_file_exits "$1" )
then
 echo "Arquivo encontrado"
else
 echo "Arquivo não encontrado"
fi

