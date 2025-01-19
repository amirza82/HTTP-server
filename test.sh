#!/bin/bash
src_path="/tmp/sources"
get_path="/tmp/servrecv"
post_path="/tmp/servpost"
white='\e[0;37m'
red='\e[0;31m'
green='\e[0;32m'

if test ! -d $src_path; then
    mkdir -p $src_path
    #prepare tmp files
    for i in $(seq 9); do
        touch "${src_path}/test$i"
        for j in $(seq $i); do
            cat server.c >> "${src_path}/test$i"
        done
    done
    echo test files are ready
fi

mkdir -p $get_path
mkdir -p $post_path

#test 1
echo -n test 1:
curl --max-time 100 -s "localhost:8080${src_path}/test3" --output "${get_path}/test"
if cmp -s ${src_path}/test3 ${get_path}/test; then
    echo -n -e " ${green}pass"
else
    echo -n -e " ${red}failed"
fi
echo -e " ${white}(reciving 1 file with GET request)"

#test 2
result=" ${green}pass"
echo -n test 2:
for file in $(ls $src_path); do
    curl --max-time 500 -s "localhost:8080$src_path/$file" --output "$get_path/$file" &
done
sleep 1
for file in $(ls $src_path); do
    if ! cmp -s $src_path/$file $get_path/$file; then
        result=" ${red}failed"
        break
    fi
done
echo -n -e "$result"
echo -e " ${white}(reciving 9 files with GET request)"

#test 3
echo -n test 3:
curl --max-time 100 -s -X POST "localhost:8080${post_path}/test" --data-binary @"$src_path/test3"
if cmp -s ${src_path}/test3 ${post_path}/test; then
    echo -n -e " ${green}pass"
else
    echo -n -e " ${red}failed"
fi
echo -e " ${white}(sending 1 file with POST request)"

#test 4
result=" ${green}pass"
echo -n "test 4:"
for file in $(ls $src_path); do
    curl --max-time 500 -s -X POST "localhost:8080$post_path/$file" --data-binary @"$src_path/$file" &
done
sleep 1
for file in $(ls $src_path); do
    if ! cmp -s $src_path/$file $post_path/$file; then
        result=" ${red}failed"
        break
    fi
done
echo -n -e "$result"
echo -e " ${white}(sending 9 files with POST request)"

rm -r $post_path $get_path
