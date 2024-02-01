#!/bin/bash
#SBATCH -J test
##SBATCH --mail-type=END
#SBATCH -e %x.err
#SBATCH -o %x.out
#SBATCH -n 98
#SBATCH --mem-per-cpu=3800   
#SBATCH -t 00:01:00
#SBATCH -t 00:10:00
module pure 
source ~/loads 

#srun mpirun  ./asyncIO.out  
srun  ./test 

EXITSTATUS=$?
#mv resultFile ${resultDir}
#rm *.tmp
echo “Job $SLURM_JOB_ID has finished at $(date).”
exit $EXITSTATUS


