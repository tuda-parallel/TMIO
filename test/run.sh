#------------------------------------------------------------------------------------------------------------
# Settings
#------------------------------------------------------------------------------------------------------------

# Output folder location
Folder=/home/av53jyqe/git/tmio/simulations/test_sim/mode6

# Executable name
filename=test

# Additional command to run before job submission
specialComand="make clean build"

# Nodes
nodes=(1 24 48 96 192 384 768 1536 3072 6144 12288 24576)

#------------------------------------------------------------------------------------------------------------
# Core function
#------------------------------------------------------------------------------------------------------------
#source ~/git/scripts/runCore/runCore.sh
source ~/git/scripts/runCore/runCore_tmio.sh
#source ~/git/scripts/runCore/runCore_scorep.sh


#------------------------------------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------------------------------------
#source ~/git/scripts/runMain/runMain.sh
source ~/git/scripts/runMain/runMain_direct.sh

