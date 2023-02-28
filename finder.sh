#!/bin/bash

declare -A tldArray
for tld in "$@"
do
  tldArray[$tld]=0
done

for f in output/*;
do
  while IFS= read -r line
  do
    words=($line)
    for tld in "$@"
    do
      if [[ ${words[0]} == *$tld ]]
      then
        tldArray[$tld]=$(( ${tldArray[$tld]} + ${words[1]} ))
      fi
    done
  done < "$f"
done

for i in "${!tldArray[@]}"
do
  echo "$i --> ${tldArray[$i]}"
done
