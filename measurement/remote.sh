#!/bin/bash

export PRIMARY=khoury-vdi-070
export BACKUP1=khoury-vdi-069
export BACKUP2=khoury-vdi-068
export BACKUP3=khoury-vdi-067
export CLIENT1=khoury-vdi-031

export PRIMARY_ADDR=`ssh $PRIMARY hostname -I | awk '{print \$1}'`
export PRIMARY_PORT=55555
export BACKUP1_ADDR=`ssh $BACKUP1 hostname -I | awk '{print \$1}'`
export BACKUP1_PORT=55555
export BACKUP2_ADDR=`ssh $BACKUP2 hostname -I | awk '{print \$1}'`
export BACKUP2_PORT=55555
export BACKUP3_ADDR=`ssh $BACKUP3 hostname -I | awk '{print \$1}'`
export BACKUP3_PORT=55555
export CLIENT1_ADDR=`ssh $CLIENT1 hostname -I | awk '{print \$1}'`
export CLIENT1_PORT=55555

echo "primary: $PRIMARY_ADDR:$PRIMARY_PORT"
echo "backup1: $BACKUP1_ADDR:$BACKUP1_PORT"
echo "backup2: $BACKUP2_ADDR:$BACKUP2_PORT"
echo "backup3: $BACKUP3_ADDR:$BACKUP3_PORT"
echo "client1: $CLIENT1_ADDR:$CLIENT1_PORT"