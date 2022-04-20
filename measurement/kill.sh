#!/bin/bash

source remote.sh

ssh $PRIMARY "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"
ssh $BACKUP1 "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"
ssh $BACKUP2 "ps -ef | grep primary_backup_server | grep -v grep | awk '{print \$2}' | xargs kill"
ssh $CLIENT1 "ps -ef | grep primary_backup_client | grep -v grep | awk '{print \$2}' | xargs kill"