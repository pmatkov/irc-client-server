#!/bin/bash

nicknames=("john" "anne" "mark")
user_infos=("jjames jjames.irc.com * :John James" "asmith asmith.irc.com * :Anne Smith" "mstevens mstevens.irc.com * :Mark Stevens")

fifo_prefix="/tmp/nc_fifo_"

create_fifo() {
    local fifo_id=$1
    local fifo_name="$fifo_prefix$fifo_id"
    mkfifo "$fifo_name"
}

send_message() {
    local command=$1
    local msg_param=$2
    local fifo_id=$3
    echo "$command $msg_param" > $fifo_prefix$fifo_id
}

create_connection() {
    local fifo_id=$1

    nc -C localhost 50100 <> $fifo_prefix$fifo_id &
}

register_user() {
    local fifo_id=$1
    local nickname=${nicknames[$1]}
    local user_info=${user_infos[$1]}

    send_message "NICK" $nickname $fifo_id
    send_message "USER" "$user_info" $fifo_id
}

close_connection() {
    local fifo_id=$1
    send_message "QUIT" ":bye" $fifo_id
}

for i in {0..2}
do
    create_fifo $i
    create_connection $i
    register_user $i
    send_message "JOIN" "#general" $i

done

send_message "PRIVMSG" "#general :hello anne" 0
send_message "PRIVMSG" "#general :hello john" 1

sleep 1

for i in {0..2}
do
    close_connection $i
    rm $fifo_prefix$i  
done
