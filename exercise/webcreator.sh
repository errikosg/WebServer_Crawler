#!/bin/bash

function checkArray() {
   	local msg="$1"   				
   	shift           				# Shift all arguments to the left (original $1 gets lost)
   	local arr=("$@")
	local i=0

   	for i in "${arr[@]}";
   	do
   		if [ "$msg" == "$i" ]
		then
			return 1
		fi
   	done
	return 0
}

#-------------------------------------------------------------------------------------------------------
#check input

if [ "$#" -lt 4 ]
	then  printf "Wrong arguments given.\nSample: ./webcreator.sh root_directory text_file w p\n.Exiting...\n"			#check number of arguments given
	exit
fi

if [ ! -e $1 ]
	then echo "Root directory given doesnt exist. Creating..."
	mkdir -p ./root_dir
fi

if [ ! -e $2 ]
	then echo "Text file given doesnt exist. Exiting..."		# check if root dir or text file exists.
	exit
fi

#check if given numbers are integers
if [ $3 -eq $3 2>/dev/null ]
then
     echo "$3 is an integer">/dev/null
else
    echo "Input for 'w' is not an integer.Exiting..."
	exit
fi

if [ $4 -eq $4 2>/dev/null ]
then
     echo "$4 is an integer">/dev/null
else
    echo "Input for 'p' is not an integer.Exiting..."
	exit
fi

root_dir="$1"
text_file="$2"
w="$3"
p="$4"

if [ "$w" -le "1" ] || [ "$p" -le "1" ] 
	then echo "Please give positive numbers only. (p must be greater than 1)"
	exit
fi

#check if text file at least 10.000 lines
text_size=`wc -l $text_file | cut -f1,1 -d ' '`
if [ $text_size -lt "10000" ]
then
	echo "Error: Text file given has less than 10000 lines"
	exit
fi

echo -e "Text size = $text_size\n\n"
let "page_count=w*p"
unique_p=()
count=0
#-----------------------------------------------------------

#clearing root directory if full
if [ "$(ls -A $root_dir)" ]; then
	echo "# Warning: $root_dir is full. Purging ..."

	rm -rf $root_dir/*
else
	echo "# $root_dir is empty. Starting webcreator."
fi

pos=0
#for every directory(website)
for ((i=0;i < $w;i++))
do
	web_path="$root_dir/site$i"
	
	#name every web page
	for((j=0;j < $p;j++))
	do
		rand="$[ ($RANDOM % 8999) + 1000 ]"
		page_path[pos]="$web_path/page$i-$rand.html"
		let "pos=pos+1"
	done
done

pos=0
index=0
for ((i=0;i < $w;i++))
do
	web_path="$root_dir/site$i"
	echo "# Creating web site $i"
	mkdir $web_path
	
	# create every web page
	for((j=0;j < $p;j++))
	do
		echo -e "\n#\tCreating page ${page_path[pos]}"
		touch ${page_path[pos]}

		k="$[ ($RANDOM % ($text_size-2002)) + 2]"
		m="$[ ($RANDOM % 999) + 1001 ]"
		let "f=(p / 2) + 1"
		let "q=(w / 2) + 1"

		internal=()
		external=()
		# make arrays for internal and then external links(f,q)
		for((l=0;l < $f;l++))
		do
			temp="$[ ($RANDOM % $p) + $index ]"
			checkArray "${page_path[temp]}" "${internal[@]}"
			return_v="$?"
			if [ "$f" -lt "$p" ]
			then
				while [ "$return_v" -eq 1 ] || [ ${page_path[pos]} == ${page_path[temp]} ]
				do
					temp="$[ ($RANDOM % $p) + $index ]"
					checkArray "${page_path[temp]}" "${internal[@]}"
					return_v="$?"
				done
			else
				while [ "$return_v" -eq 1 ]
				do
					temp="$[ ($RANDOM % $p) + $index ]"
					checkArray "${page_path[temp]}" "${internal[@]}"
					return_v="$?"
				done
			fi

			internal[l]=${page_path[temp]}

			#keep the unique paths for ending message about links
			checkArray "${internal[l]}" "${unique_p[@]}"
			v="$?"
			if [ "$v" -eq 0 ]
			then
				unique_p[count]=${internal[l]}
				let "count=count+1"
			fi

			#echo -e "#\t\t--made link(internal) to ${page_path[pos]}:  ${internal[l]}"
		done
		y=0
		for((l=0;l < $q;l++))
		do
			for((z=0;z < $page_count;z++))
			do																#make temp array
				x=`echo ${page_path[z]} | cut -f3,3  -d '/'`
				if [ "$x" != "site$i" ]
				then
					temp_array[y]="${page_path[z]}"
					let "y=y+1"
				fi
			done

			temp="$[ ($RANDOM % ($page_count-$p))]"
			checkArray "${temp_array[temp]}" "${external[@]}"
			return_v="$?"
			while [ "$return_v" -eq 1 ]
			do
				temp="$[ ($RANDOM % ($page_count-$p))]"
				checkArray "${temp_array[temp]}" "${external[@]}"
				return_v="$?"
			done
			external[l]=${temp_array[temp]}

			#keep the unique paths for ending message about links
			checkArray "${external[l]}" "${unique_p[@]}"
			v="$?"
			if [ "$v" -eq 0 ]
			then
				unique_p[count]=${external[l]}
				let "count=count+1"
			fi

			#echo -e "#\t\t--made link(external) to ${page_path[pos]}:  ${external[l]}"
		done

		# write actual .html file
		#-first write headers(step 5), then info(step6+7), then close tags

		#echo -e "<!DOCTYPE html>\n<head><meta charset='UTF-8'></head>\n\n<html>\n<body>" > ${page_path[pos]}
		printf "<!DOCTYPE html>\n<head><meta charset='UTF-8'></head>\n<html>\n<body>" > ${page_path[pos]}
		l=0
		z=0
		x="$k"
		let "value=m/(f+q)"
		for((b=0;b < ($f+$q);b++))
		do
			#write m/(f+q) lines = paragraph($value)
			#echo -ne "\n<h3>Text No$b</h3>\n\n<p>" >> ${page_path[pos]}
			printf "\n<h3>Text No$b</h3>\n<pre>" >> ${page_path[pos]}

			while read -r line
			do
				#echo -e "$line<br>" >> ${page_path[pos]}
				#printf "%s\n" "$line" >> ${page_path[pos]}
				echo -n "$line" >> ${page_path[pos]}
				printf "\n" >> ${page_path[pos]}
			done < <(tail $text_file -n +$x | head -n $value)
			let "x=x+value"

			#write the internal link before the external
			if [ "$l" -lt "$f" ]
			then
				eric=`echo ${internal[l]} | cut -f3,4 -d '/'`
				temp="../$eric"
				#echo -e "<a href='$temp'>link$b-text</a></p>" >> ${page_path[pos]}
				printf "<a href='%s'>link%s-text</a></pre>" "$temp" "$b" >> ${page_path[pos]}
				echo -e "#\tAdding link(internal) to ${page_path[pos]}:  ${internal[l]}"
				let "l=l+1"
			else
				#temp="../../${external[z]}"
				eric=`echo ${external[z]} | cut -f3,4 -d '/'`
				temp="../$eric"
				#echo -e "<a href='$temp' target='_blank'>link$b-text</a></p>" >> ${page_path[pos]}
				printf "<a href='%s' target='_blank'>link%s-text</a></pre>" "$temp" "$b" >> ${page_path[pos]}
				echo -e "#\tAdding link(external) to ${page_path[pos]}:  ${external[z]}"
				let "z=z+1"
			fi
		done
		#echo -e "\n\n</body>\n</html>" >> ${page_path[pos]}
		printf "\n</body>\n</html>\n" >> ${page_path[pos]}

		let "pos=pos+1"
	done
	let "index=$index+$p"
done

if [ "$count" -eq "$page_count" ]
then
	echo -e "\n# All pages have at least one incoming link"
else
	echo -e "\n# Not(!) all pages have at least one incoming link"
fi

echo "# Done"

