#!groovy

def jobMatrix(String type, List specs, Closure callback) {
  def nodes = [:]
  for (spec in specs) {
    if (type == 'macos') {
      def label = "${spec.type}-${spec.os}-FairSoft_${spec.fairsoft}-${env.JOB_BASE_NAME}"
      nodes["${label}"] = {
        node("${spec.os}-${spec.fairsoft}") {
          githubNotify(context: "${label}", description: 'Building ...', status: 'PENDING')
          try {
            deleteDir()
            checkout scm

            callback.call(spec, label)

            deleteDir()
            githubNotify(context: "${label}", description: 'Success', status: 'SUCCESS')
          } catch (e) {
            def tarball = "build_${label}_dds_logs.tar.gz"
            sh "tar czvf ${tarball} -C \${WORKSPACE}/build/test/ .DDS/"
            archiveArtifacts tarball

            deleteDir()
            githubNotify(context: "${label}", description: 'Error', status: 'ERROR')
            throw e
          }
        }
      }
    } else {
      def label = "${spec.type}-${spec.os}-${spec.compiler}-${env.JOB_BASE_NAME}"
      nodes["${label}"] = {
        node("slurm") {
          githubNotify(context: "${label}", description: 'Building ...', status: 'PENDING')
          def tarball = "${label}_dds_logs.tar.gz"
          try {
            deleteDir()
            checkout scm
            sh 'mktemp -d -p /dev/shm > tmpdir'
            sh """\
              echo "#!/bin/bash" > job.sh
              echo "set -x" >> job.sh
              echo "cleanup() {" >> job.sh
              echo "  rm -rf \\\$1" >> job.sh
              echo "}" >> job.sh
              echo "env" >> job.sh
              echo "tmp=\$(cat ./tmpdir)" >> job.sh
              echo "trap 'cleanup \\\$tmp' EXIT" >> job.sh
              echo "cp -r . \\\$tmp/" >> job.sh
              echo "cd \\\$tmp" >> job.sh
              echo "img=${env.SINGULARITY_CONTAINER_ROOT}/fairmq/${spec.container}" >> job.sh
              echo "cp \\\$img \\\$tmp/" >> job.sh
              echo "singularity exec \\\$tmp/${spec.container} bash -c \\\"LD_LIBRARY_PATH=/usr/lib ctest -DCTEST_BUILD_NAME='${label}' -DCTEST_MODEL=${spec.track} -DCMAKE_BUILD_TYPE=${spec.type} -DCTEST_SITE=${env.CTEST_SITE} -V -S test/FairMQTest.cmake\\\"" >> job.sh
            """
            sh 'cat job.sh'
            sh 'env'
            callback.call(spec, label)
            deleteDir()
            githubNotify(context: "${label}", description: 'Success', status: 'SUCCESS')
          } catch (e) {
            deleteDir()
            githubNotify(context: "${label}", description: 'Error', status: 'ERROR')
            throw e
          }
        }
      }
    }
  }
  return nodes
}

pipeline{
  agent none
  stages {
    stage("Run checks") {
      steps{
        script {
          def mac_jobs = jobMatrix('macos', [
            [os: 'macOS10.13', arch: 'x86_64', fairsoft: 'fairmq_dev', type: 'RelWithDebInfo'],
            [os: 'macOS10.14', arch: 'x86_64', fairsoft: 'fairmq_dev', type: 'RelWithDebInfo'],
          ]) { spec, label ->
            sh "ctest -DCTEST_BUILD_NAME='${label}' -DCTEST_MODEL=Continuous -DCTEST_CMAKE_GENERATOR='Unix Makefiles' -DCMAKE_BUILD_TYPE=${spec.type} -DCMAKE_INSTALL_PREFIX=${env.SIMPATH_PREFIX}${spec.fairsoft} -DBUILD_PMIX_PLUGIN=OFF -V -S test/FairMQTest.cmake"
          }

          def linux_jobs = jobMatrix('virgo', [
            [os: 'Fedora31', arch: 'x86_64', compiler: 'gcc9.2', container: 'fedora.31.sif', track: 'Continuous', type: 'RelWithDebInfo'],
            [os: 'Fedora31', arch: 'x86_64', compiler: 'gcc9.2', container: 'fedora.31.sif', track: 'Coverage'  , type: 'Profile'],
          ]) { spec, label ->
            sh "srun -p main -c 16 -n 1 -t 30:00 --job-name='alfaci-${label}' bash job.sh"
          }

          parallel(mac_jobs + linux_jobs)
        }
      }
    }
  }
}
