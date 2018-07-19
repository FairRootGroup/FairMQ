#!/bin/bash


function print_example(){
echo "##################################################################"
echo "# To set the required parameters as source and the build         #"
echo "# directory for ctest, the linux flavour and the SIMPATH         #"
echo "# put the export commands below to a separate file which is read #"
echo "# during execution and which is defined on the command line.     #"
echo "# Set all parameters according to your needs.                    #"
echo "# LINUX_FLAVOUR should be set to the distribution you are using  #"
echo "# eg Debian, SuSe etc.                                           #"
echo "# An additional varibale NCPU can overwrite the default number   #"
echo "# of parallel processes used to compile the project.             #"
echo "# This can be usefull if one can use a distributed build system  #"
echo "# like icecream.                                                 #"
echo "# For example                                                    #"
echo "#!/bin/bash                                                      #"
echo "export LINUX_FLAVOUR=<your linux flavour>                        #"
echo "export FAIRSOFT_VERSION=<version of FairSoft>                    #"
echo "export SIMPATH=<path to your FairSoft version>                   #"
echo "export GIT_BRANCH=< master or dev>                               #"
echo "export BUILDDIR=<dir where the build files go>                   #"
echo "export SOURCEDIR=<location of the FairRoot sources>              #"
echo "#export NCPU=100                                                 #"
echo "##################################################################"
}

if [ "$#" -lt "2" ]; then
  echo ""
  echo "-- Error -- Please start script with two parameters"
  echo "-- Error -- The first parameter is the ctest model."
  echo "-- Error -- Possible arguments are Nightly, Experimental, "
  echo "-- Error -- Continuous or Profile."
  echo "-- Error -- The second parameter is the file containg the"
  echo "-- Error -- Information about the setup at the client"
  echo "-- Error -- installation (see example below)."
  echo ""
  print_example
  exit 1
fi

# test if a valid ctest model is defined
case "$1" in
  Experimental|Nightly|Continuous|Profile|alfa_ci|codecov)
    ;;
  *)
    echo "-- Error -- This ctest model is not supported."
    echo "-- Error -- Possible arguments are Nightly, Experimental, Continuous or Profile."
    exit 1
    ;;
esac

# test if the input file exists and execute it
if [ -e "$2" ];then
  source $2
else
  echo "-- Error -- Input file does not exist."
  echo "-- Error -- Please choose existing input file."
  exit 1
fi

# set the ctest model to command line parameter
if [ "$1" == "alfa_ci" ]; then
  export ctest_model=Experimental
elif [ "$1" == "codecov" ]; then
  export ctest_model=Profile
  export do_codecov_upload=1
else
  export ctest_model=$1
fi


# test for architecture
arch=$(uname -s | tr '[A-Z]' '[a-z]')
chip=$(uname -m | tr '[A-Z]' '[a-z]')

# extract information about the system and the machine and set
# environment variables used by ctest
SYSTEM=$arch-$chip
if test -z $CXX ; then
  if [ "$arch" == "darwin" ]; then
    COMPILER=$(clang --version | head -n 1 | cut -d' ' -f1,2,4 | tr -d ' ')
  else
    COMPILER=gcc$(gcc -dumpversion)
  fi
else
  COMPILER=$CXX$($CXX -dumpversion)
fi

case "$1" in
  alfa_ci)
    export LABEL1=alfa_ci-$COMPILER-FairMQ_$GIT_BRANCH
    export LABEL=$(echo $LABEL1 | sed -e 's#/#_#g')
    ;;
  codecov)
    export LABEL1=codecov-$COMPILER-FairMQ_$GIT_BRANCH
    export LABEL=$(echo $LABEL1 | sed -e 's#/#_#g')
    ;;
  *)
    export LABEL1=${LINUX_FLAVOUR}-$chip-$COMPILER-FairMQ_$GIT_BRANCH
    export LABEL=$(echo $LABEL1 | sed -e 's#/#_#g')
    ;;
esac

# get the number of processors
# and information about the host
if [ "$arch" = "linux" ];
then
  if [ "$NCPU" != "" ];
  then
    export number_of_processors=$NCPU
  else
    export number_of_processors=$(cat /proc/cpuinfo | grep processor | wc -l)
  fi
  if [ -z "$SITE" ]; then
    export SITE=$(hostname -f)
    if [ -z "$SITE" ]; then
      export SITE=$(uname -n)
    fi
  fi
elif [ "$arch" = "darwin" ];
then
  if [ "$NCPU" != "" ];
  then
    export number_of_processors=$NCPU
  else
    export number_of_processors=$(sysctl -n hw.ncpu)
  fi
  if [ -z "$SITE" ]; then
    export SITE=$(hostname -s)
  fi
fi

echo "************************"
date
echo "LABEL: " $LABEL
echo "SITE: " $SITE
echo "Model: " ${ctest_model}
echo "Nr. of processes: " $number_of_processors
echo "************************"

cd $SOURCEDIR

ctest -S FairMQTest.cmake -V --VV
