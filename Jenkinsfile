#!groovy

def specToLabel(Map spec) {
  return "${spec.os}-${spec.arch}-${spec.compiler}-FairSoft_${spec.fairsoft}"
}

def jobMatrix(String prefix, List specs, Closure callback) {
  def nodes = [:]
  for (spec in specs) {
    def label = specToLabel(spec)
    def node_tag = label
    if (spec.os =~ /macOS/) {
      node_tag = spec.os
    }
    def fairsoft = spec.fairsoft
    def os = spec.os
    def compiler = spec.compiler
    nodes["${prefix}/${label}"] = {
      node(node_tag) {
        githubNotify(context: "${prefix}/${label}", description: 'Building ...', status: 'PENDING')
        try {
          deleteDir()
          checkout scm

          sh """\
            echo "export SIMPATH=\${SIMPATH_PREFIX}${fairsoft}" >> Dart.cfg
            echo "export FAIRSOFT_VERSION=${fairsoft}" >> Dart.cfg
          """
          if (os =~ /Debian/ && compiler =~ /gcc9/) {
            sh '''\
              echo "source /etc/profile.d/modules.sh" >> Dart.cfg
              echo "module use /cvmfs/it.gsi.de/modulefiles" >> Dart.cfg
              echo "module load compiler/gcc/9.1.0" >> Dart.cfg
            '''
          }
          if (os =~ /[Mm]acOS/) {
            sh "echo \"export EXTRA_FLAGS='-DCMAKE_CXX_COMPILER=clang++'\" >> Dart.cfg"
          } else {
            sh "echo \"export EXTRA_FLAGS='-DCMAKE_CXX_COMPILER=g++'\" >> Dart.cfg"
          }

          sh '''\
            echo "export BUILDDIR=$PWD/build" >> Dart.cfg
            echo "export SOURCEDIR=$PWD" >> Dart.cfg
            echo "export PATH=\\\$SIMPATH/bin:\\\$PATH" >> Dart.cfg
            echo "export GIT_BRANCH=$JOB_BASE_NAME" >> Dart.cfg
            echo "echo \\\$PATH" >> Dart.cfg
          '''

          if (os =~ /macOS10.14/) {
            sh "echo \"export EXCLUDE_UNSTABLE_DDS_TESTS=1\" >> Dart.cfg"
          }

          sh 'cat Dart.cfg'

          callback.call(spec, label)

          deleteDir()
          githubNotify(context: "${prefix}/${label}", description: 'Success', status: 'SUCCESS')
        } catch (e) {
          def tarball = "${prefix}_${label}_dds_logs.tar.gz"
          sh "tar czvf ${tarball} -C \${WORKSPACE}/build/test/ .DDS/"
          archiveArtifacts tarball

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
          def build_jobs = jobMatrix('build', [
            [os: 'Debian8',    arch: 'x86_64', compiler: 'gcc9.1.0',       fairsoft: 'fairmq_dev'],
            [os: 'macOS10.14', arch: 'x86_64', compiler: 'AppleClang11.0', fairsoft: 'fairmq_dev'],
            [os: 'macOS10.15', arch: 'x86_64', compiler: 'AppleClang12.0', fairsoft: 'fairmq_dev'],
          ]) { spec, label ->
            sh './Dart.sh alfa_ci Dart.cfg'
          }

          def profile_jobs = jobMatrix('codecov', [
            [os: 'Debian8',    arch: 'x86_64', compiler: 'gcc9.1.0',        fairsoft: 'fairmq_dev'],
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
