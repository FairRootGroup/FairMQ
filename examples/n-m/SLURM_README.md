# Running FairMQ n-m Example on Slurm

This guide explains how to run the n-m example topology on a Slurm-managed cluster.

## For GSI vae25 Cluster Users

### Accessing the Cluster

1. **Connect to the submit node:**
   ```bash
   ssh vae25.hpc.gsi.de
   ```

   You'll see a message indicating the container has been launched:
   ```
   Slurm Cluster – Virgo 3.0 Submit Node – Manual: https://hpc.gsi.de/virgo
   Container launched: /cvmfs/vae.gsi.de/vae25/containers/vae25-user_container_20250827T1311.sif
   ```

2. **Transfer the Slurm script to the cluster:**

   From your local machine:
   ```bash
   scp fairmq-start-ex-n-m-slurm.sh vae25.hpc.gsi.de:~/
   ```

   Or create it directly on the cluster using your preferred editor.

3. **Copy the script to your lustre workspace:**

   **Important:** The home directory (`/u/username/`) may not be accessible from compute nodes within the container. Use the shared lustre filesystem instead:

   ```bash
   cp fairmq-start-ex-n-m-slurm.sh /lustre/rz/$USER/
   cd /lustre/rz/$USER/
   ```

5. **Verify FairMQ is available:**
   ```bash
   ls /cvmfs/fairsoft.gsi.de/debian12/fairsoft/jan24p5/bin/fairmq-ex-n-m-*
   ```

   You should see the three executables we need:
   - `fairmq-ex-n-m-synchronizer`
   - `fairmq-ex-n-m-sender`
   - `fairmq-ex-n-m-receiver`

6. **Submit the job:**
   ```bash
   sbatch fairmq-start-ex-n-m-slurm.sh
   ```

   Monitor with:
   ```bash
   squeue -u $USER
   ```

**Notes:**
- The vae25 container is automatically loaded on all compute nodes, so no container configuration is needed in the script.
- The script uses the `main` partition by default (8 hour time limit). Other available partitions: `debug` (30 min), `grid` (3 days), `long` (7 days), `high_mem`, `gpu`.

**Cluster Documentation:** https://hpc.gsi.de/virgo

## General Prerequisites

1. FairMQ must be built and installed (or available via CVMFS as on vae25)
2. The executables must be accessible on all compute nodes
3. Access to a Slurm cluster with at least 8 nodes

## Quick Start (General)

### Submit the job to Slurm:
```bash
sbatch fairmq-start-ex-n-m-slurm.sh
```

### Check job status:
```bash
squeue -u $USER
```

### View output (replace JOBID with your job ID):
```bash
# Get your job ID from squeue, then:
tail -f fairmq-n-m-JOBID.out

# Or check the latest output file:
tail -f fairmq-n-m-*.out
```

### Cancel the job:
```bash
scancel JOBID
```

## Customization

### For GSI vae25 Cluster

The script is pre-configured to use:
```bash
FAIRSOFT_BIN="/cvmfs/fairsoft.gsi.de/debian12/fairsoft/jan24p5/bin"
```

If you're using a different FairSoft version on CVMFS, update this path.

### General Configuration

You can modify the following parameters in the script:

#### Resource Allocation
Edit the SBATCH directives at the top of the script:

```bash
#SBATCH --partition=main       # Partition (main, debug, grid, long, high_mem, gpu)
#SBATCH --nodes=8              # Total nodes needed (1 sync + N senders + M receivers)
#SBATCH --ntasks=8             # Total tasks
#SBATCH --time=01:00:00        # Wall time limit
```

**GSI vae25 partition limits:**
- `debug`: 30 minutes, 8 nodes max
- `main`: 8 hours (default)
- `grid`: 3 days
- `long`: 7 days

#### Topology Configuration
Edit the configuration variables in the script:

```bash
NUM_SENDERS=3                  # Number of sender devices
NUM_RECEIVERS=4                # Number of receiver devices
SUBTIMEFRAME_SIZE=1000000      # Size of subtimeframes in bytes
RATE=100                       # Rate of synchronizer in Hz
```

**Important:** If you change NUM_SENDERS or NUM_RECEIVERS, you must also update the SBATCH `--nodes` and `--ntasks` parameters to match: `nodes = 1 + NUM_SENDERS + NUM_RECEIVERS`

#### Port Configuration

The script uses the following default ports:
- Synchronizer: 8010
- Receivers: 8021-8024 (incremental based on NUM_RECEIVERS)

You can modify these by editing:
```bash
SYNC_PORT=8010
RECEIVER_BASE_PORT=8021
```

## How It Works

### Node Allocation
The script allocates nodes in the following order:
1. **Node 0**: Synchronizer
2. **Nodes 1-3**: Senders
3. **Nodes 4-7**: Receivers

### Device Startup Order
All devices are started in parallel:
1. **Synchronizer** binds to port 8010
2. **Receivers** bind to their respective ports (8021-8024)
3. **Senders** connect to the synchronizer and all receivers

ZeroMQ handles the bind/connect establishment automatically, so the startup order doesn't matter. Devices can start in any order and will establish connections when both sides are ready.

### Communication Pattern

```
Synchronizer (PUB)
    |
    | sync messages
    v
Sender 1, 2, 3 (SUB -> PUSH)
    |
    | data distribution based on message ID
    v
Receiver 1, 2, 3, 4 (PULL)
```

- The synchronizer publishes sync messages via PUB/SUB pattern
- Each sender subscribes to sync messages
- Senders distribute data to receivers using PUSH/PULL pattern
- Data is routed to specific receivers based on the ID in the sync message

## Running on Fewer Nodes

If you want to run multiple devices per node (e.g., for testing on a small cluster):

```bash
#SBATCH --nodes=4              # Use 4 nodes instead of 8
#SBATCH --ntasks=8             # Still 8 tasks total
#SBATCH --ntasks-per-node=2    # 2 tasks per node
```

Note: This may have performance implications due to shared resources.

## Troubleshooting

### Issue: "Unable to allocate resources"
**Solution:** Reduce the number of requested nodes or check cluster availability with `sinfo`

### Issue: Job fails immediately with "couldn't chdir" error
**Symptoms:**
```
slurmstepd: error: couldn't chdir to `/u/username/...': No such file or directory
```

**Solution:**
- The home directory is not accessible from compute nodes within the container
- Copy your script to the lustre filesystem: `/lustre/rz/$USER/`
- Submit the job from there

### Issue: Devices can't connect
**Solution:**
- Check that firewall rules allow communication between nodes
- Verify hostnames are resolvable between nodes
- Check the output log for specific error messages

### Issue: Port already in use
**Solution:**
- Change `SYNC_PORT` and `RECEIVER_BASE_PORT` to unused ports
- Wait for previous job to fully terminate

### Issue: Devices exit immediately
**Solution:**
- Check that FairMQ executables are in PATH on all compute nodes
- Verify the build was successful and executables exist
- Use `--control static` mode to prevent interactive state machine (already set in script)

## Advanced: Interactive Mode

To run interactively for debugging (allocates resources and gives you a shell):

```bash
salloc --nodes=8 --ntasks=8 --time=01:00:00
# Then run the script content manually or modify for interactive use
```

## Monitoring

While the job is running:

```bash
# Watch job status
watch -n 1 squeue -u $USER

# Monitor output in real-time
tail -f fairmq-n-m-JOBID.out

# Check resource usage
sstat -j JOBID

# After completion, view accounting info
sacct -j JOBID --format=JobID,JobName,Partition,State,Elapsed,MaxRSS
```
