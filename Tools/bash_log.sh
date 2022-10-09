#! /usr/bin/env bash

cnf_log_level=0
###
debug_log_level=0
info_log_level=1
warn_log_level=2
error_log_level=3

function log() {
    local log_type="$1"
    local func_name="$2"
    local line_no="$3"
    local file_name="$4"
    local data_info="$5"
    date_time=$(date +"%F %H:%M:%S")
    if [[ -z ${data_info} ]]; then
        data_format="[${date_time}] [${log_type}] ${file_name} ${func_name}:${line_no}"
    else
        data_format="[${date_time}] [${log_type}] ${file_name} ${func_name}:${line_no},"
    fi

    case ${log_type} in
        "debug")
                [[ $cnf_log_level -le ${debug_log_level} ]] && echo -e "\033[30m${data_format} ${data_info}\033[0m"
                ;;
        "info")
                [[ $cnf_log_level -le ${info_log_level} ]] && echo -e "\033[32m${data_format} ${data_info}\033[0m"
                ;;
        "warn")
                [[ $cnf_log_level -le ${warn_log_level} ]] && echo -e "\033[33m${data_format} ${data_info}\033[0m"
                ;;
        "error")
                [[ $cnf_log_level -le ${error_log_level} ]] && echo -e "\033[31m${data_format} ${data_info}\033[0m"
                ;;
    esac
}

function get_line() {
	log_info=$(caller 1)
	line_no=$(echo ${log_info}|awk '{print $1}')
	echo $line_no
}

#########################################
#
# 对外暴露接口, 使用实例:
#  LOG_DEBUG aaa bbb cc 123
##########################################
function LOG_DEBUG() {
	line_no=$(get_line)
	log "debug" ${FUNCNAME[1]} "${line_no}" "$(basename ${BASH_SOURCE[1]})" "$*"
}

function LOG_INFO() {
	line_no=$(get_line)
	log "info" ${FUNCNAME[1]} "${line_no}" "$(basename ${BASH_SOURCE[1]})" "$*"
}

function LOG_WARN() {
	line_no=$(get_line)
	log "warn" ${FUNCNAME[1]} "${line_no}" "$(basename ${BASH_SOURCE[1]})" "$*"
}
function LOG_ERROR() {
	line_no=$(get_line)
	log "error" ${FUNCNAME[1]} "${line_no}" "$(basename ${BASH_SOURCE[1]})" "$*"
}
