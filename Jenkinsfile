#!groovy

def specToLabel(Map spec) {
  return "${spec.os}-${spec.arch}-${spec.compiler}-FairSoft_${spec.fairsoft}"
}

def jobMatrix(String prefix, List specs, Closure callback) {
  def nodes = [:]
  for (spec in specs) {
    def label = specToLabel(spec)
    nodes["${prefix}/${label}"] = {
      node(label) {
        githubNotify(context: "${prefix}/${label}", description: 'Building ...', status: 'PENDING')
        try {
          deleteDir()
          checkout scm
          
          sh """\
            echo "export SIMPATH=\${SIMPATH_PREFIX}${spec.fairsoft}" >> Dart.cfg
            echo "export FAIRSOFT_VERSION=${spec.fairsoft}" >> Dart.cfg
          """
          if ((spec.os == 'Debian8') && (spec.compiler == 'gcc8.1')) {
            sh '''\
              echo "source /etc/profile.d/modules.sh" >> Dart.cfg
              echo "module use /cvmfs/it.gsi.de/modulefiles" >> Dart.cfg
              echo "module load compiler/gcc/8" >> Dart.cfg
            '''
          }
          sh '''\
            echo "export BUILDDIR=$PWD/build" >> Dart.cfg
            echo "export SOURCEDIR=$PWD" >> Dart.cfg
            echo "export PATH=\\\$SIMPATH/bin:\\\$PATH" >> Dart.cfg
            echo "export GIT_BRANCH=$JOB_BASE_NAME" >> Dart.cfg
            echo "echo \\\$PATH" >> Dart.cfg
          '''
          sh 'cat Dart.cfg'

          callback.call(spec, label)

          deleteDir()
          githubNotify(context: "${prefix}/${label}", description: 'Success', status: 'SUCCESS')
        } catch (e) {
          deleteDir()
          githubNotify(context: "${prefix}/${label}", description: 'Error', status: 'ERROR')
          throw e
        }
      }
    }
  }
  return nodes
}

pipeline{
  agent none
  stages {
    stage("Run CI Matrix") {
      steps{
        script {
          def build_jobs = jobMatrix('alfa-ci/build', [
            [os: 'Debian8',    arch: 'x86_64', compiler: 'gcc8.1',          fairsoft: 'fairmq_dev'],
            //[os: 'MacOS10.13', arch: 'x86_64', compiler: 'AppleLLVM10.0.0', fairsoft: 'may18'],
          ]) { spec, label ->
            sh './Dart.sh alfa_ci Dart.cfg'
          }

          def profile_jobs = jobMatrix('alfa-ci/codecov', [
            [os: 'Debian8',    arch: 'x86_64', compiler: 'gcc8.1',          fairsoft: 'fairmq_dev'],
          ]) { spec, label ->
            withCredentials([string(credentialsId: 'fairmq_codecov_token', variable: 'CODECOV_TOKEN')]) {
              sh './Dart.sh codecov Dart.cfg'
            }
          }

          parallel(build_jobs + profile_jobs)
        }
      }
    }
  }
}
