#! /bin/bash

label="$1"
jobsh="$2"

if [ -z "$ALFACI_SLURM_CPUS" ]
then
	ALFACI_SLURM_CPUS=32
fi
if [ -z "$ALFACI_SLURM_EXTRA_OPTS" ]
then
	ALFACI_SLURM_EXTRA_OPTS="--hint=compute_bound"
fi
if [ -z "$ALFACI_SLURM_TIMEOUT" ]
then
	ALFACI_SLURM_TIMEOUT=30
fi
if [ -z "$ALFACI_SLURM_QUEUE" ]
then
	ALFACI_SLURM_QUEUE=main
fi

echo "*** Slurm request options :"
echo "***   Working directory ..: $PWD"
echo "***   Queue ..............: $ALFACI_SLURM_QUEUE"
echo "***   CPUs ...............: $ALFACI_SLURM_CPUS"
echo "***   Wall Time ..........: $ALFACI_SLURM_TIMEOUT min"
echo "***   Job Name ...........: ${label}"
echo "***   Extra Options ......: ${ALFACI_SLURM_EXTRA_OPTS}"
echo "*** Submitting job at ....: $(date -R)"
(
	set -x
	srun -p $ALFACI_SLURM_QUEUE -c $ALFACI_SLURM_CPUS -n 1 \
		-t $ALFACI_SLURM_TIMEOUT \
		--job-name="${label}" \
		${ALFACI_SLURM_EXTRA_OPTS} \
		bash "${jobsh}"
)
retval=$?
echo "*** Exit Code ............: $retval"
exit "$retval"
