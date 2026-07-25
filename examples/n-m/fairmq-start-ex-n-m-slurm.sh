#!/bin/bash
#SBATCH --job-name=fairmq-n-m
#SBATCH --partition=main
#SBATCH --nodes=8
#SBATCH --ntasks=8
#SBATCH --ntasks-per-node=1
#SBATCH --time=01:00:00
#SBATCH --output=fairmq-n-m-%j.out
#SBATCH --error=fairmq-n-m-%j.err

# FairMQ n-m Example for Slurm with vae25 container
# Topology: 1 synchronizer -> 3 senders -> 4 receivers
# Container is automatically loaded by the cluster

# FairSoft configuration
FAIRSOFT_BIN="/cvmfs/fairsoft.gsi.de/debian12/fairsoft/jan24p5/bin"

# Configuration
NUM_SENDERS=3
NUM_RECEIVERS=4
SUBTIMEFRAME_SIZE=1000000
RATE=100

# Base port numbers
SYNC_PORT=8010
RECEIVER_BASE_PORT=8021

# Get the list of allocated nodes
NODELIST=($(scontrol show hostname $SLURM_NODELIST))

# Assign nodes to devices
SYNC_NODE=${NODELIST[0]}
SENDER_NODES=(${NODELIST[1]} ${NODELIST[2]} ${NODELIST[3]})
RECEIVER_NODES=(${NODELIST[4]} ${NODELIST[5]} ${NODELIST[6]} ${NODELIST[7]})

echo "==========================================="
echo "FairMQ n-m Example on Slurm"
echo "==========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Synchronizer node: $SYNC_NODE"
echo "Sender nodes: ${SENDER_NODES[@]}"
echo "Receiver nodes: ${RECEIVER_NODES[@]}"
echo "==========================================="

# Build receiver addresses for senders to connect to
RECEIVER_ADDRESSES=""
for i in $(seq 0 $((NUM_RECEIVERS - 1))); do
    RECEIVER_PORT=$((RECEIVER_BASE_PORT + i))
    if [ $i -eq 0 ]; then
        RECEIVER_ADDRESSES="address=tcp://${RECEIVER_NODES[$i]}:${RECEIVER_PORT}"
    else
        RECEIVER_ADDRESSES="${RECEIVER_ADDRESSES},address=tcp://${RECEIVER_NODES[$i]}:${RECEIVER_PORT}"
    fi
done

# Start all devices in parallel (ZeroMQ handles bind/connect order automatically)
echo "Starting synchronizer on $SYNC_NODE..."
srun --nodes=1 --ntasks=1 --nodelist=$SYNC_NODE \
    ${FAIRSOFT_BIN}/fairmq-ex-n-m-synchronizer \
    --id Sync \
    --channel-config name=sync,type=pub,method=bind,address=tcp://*:${SYNC_PORT} \
    --rate ${RATE} \
    --verbosity veryhigh \
    --control static &

echo "Starting ${NUM_RECEIVERS} receivers..."
for i in $(seq 0 $((NUM_RECEIVERS - 1))); do
    RECEIVER_ID="Receiver$((i + 1))"
    RECEIVER_PORT=$((RECEIVER_BASE_PORT + i))
    RECEIVER_NODE=${RECEIVER_NODES[$i]}

    echo "  Starting $RECEIVER_ID on $RECEIVER_NODE:$RECEIVER_PORT"
    srun --nodes=1 --ntasks=1 --nodelist=$RECEIVER_NODE \
        ${FAIRSOFT_BIN}/fairmq-ex-n-m-receiver \
        --id $RECEIVER_ID \
        --channel-config name=data,type=pull,method=bind,address=tcp://*:${RECEIVER_PORT} \
        --num-senders ${NUM_SENDERS} \
        --verbosity veryhigh \
        --control static &
done

echo "Starting ${NUM_SENDERS} senders..."
for i in $(seq 0 $((NUM_SENDERS - 1))); do
    SENDER_ID="Sender$((i + 1))"
    SENDER_NODE=${SENDER_NODES[$i]}

    echo "  Starting $SENDER_ID on $SENDER_NODE"
    srun --nodes=1 --ntasks=1 --nodelist=$SENDER_NODE \
        ${FAIRSOFT_BIN}/fairmq-ex-n-m-sender \
        --id $SENDER_ID \
        --channel-config name=sync,type=sub,method=connect,address=tcp://${SYNC_NODE}:${SYNC_PORT} \
                          name=data,type=push,method=connect,${RECEIVER_ADDRESSES} \
        --sender-index $i \
        --subtimeframe-size ${SUBTIMEFRAME_SIZE} \
        --num-receivers ${NUM_RECEIVERS} \
        --verbosity veryhigh \
        --control static &
done

echo "==========================================="
echo "All devices started. Waiting for completion..."
echo "Press Ctrl+C to terminate all processes."
echo "==========================================="

# Wait for all background jobs
wait

echo "==========================================="
echo "FairMQ n-m example completed"
echo "==========================================="
