#!/bin/bash 
declare -i  min_val=1
while ((min_val < 40)); do
{
    ./AccessTest
    let ++min_val
} &
done
wait
