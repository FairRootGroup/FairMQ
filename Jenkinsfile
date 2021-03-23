#!groovy

def jobMatrix(String type, List specs) {
  def nodes = [:]
  for (spec in specs) {
    job = "${spec.os}-${spec.ver}-${spec.arch}-${spec.compiler}"
    def label = "${type}/${job}"
    def selector = "${spec.os}-${spec.ver}-${spec.arch}"
    def os = spec.os
    def ver = spec.ver
    def check = spec.check

    nodes[label] = {
      node(selector) {
        githubNotify(context: "${label}", description: 'Building ...', status: 'PENDING')
        try {
          deleteDir()
          checkout scm

          def jobscript = 'job.sh'
          def ctestcmd = "ctest -S FairMQTest.cmake -V --output-on-failure"
          sh "echo \"set -e\" >> ${jobscript}"
          sh "echo \"export LABEL=\\\"\${JOB_BASE_NAME} ${label}\\\"\" >> ${jobscript}"
          if (selector =~ /^macos/) {
            sh """\
              echo \"export DDS_ROOT=\\\"\\\$(brew --prefix dds)\\\"\" >> ${jobscript}
              echo \"${ctestcmd}\" >> ${jobscript}
            """
            sh "cat ${jobscript}"
            sh "bash ${jobscript}"
          } else {
            def containercmd = "singularity exec -B/shared ${env.SINGULARITY_CONTAINER_ROOT}/fairmq/${os}.${ver}.sif bash -l -c \\\"${ctestcmd} -DRUN_STATIC_ANALYSIS=ON\\\""
            sh """\
              echo \"echo \\\"*** Job started at .......: \\\$(date -R)\\\"\" >> ${jobscript}
              echo \"echo \\\"*** Job ID ...............: \\\${SLURM_JOB_ID}\\\"\" >> ${jobscript}
              echo \"echo \\\"*** Compute node .........: \\\$(hostname -f)\\\"\" >> ${jobscript}
              echo \"unset http_proxy\" >> ${jobscript}
              echo \"unset HTTP_PROXY\" >> ${jobscript}
              echo \"${containercmd}\" >> ${jobscript}
            """
            sh "cat ${jobscript}"
            sh "test/ci/slurm-submit.sh \"FairMQ \${JOB_BASE_NAME} ${label}\" ${jobscript}"

            withChecks('Static Analysis') {
              recordIssues(enabledForFailure: true,
                           tools: [gcc(pattern: 'build/Testing/Temporary/*.log')],
                           filters: [excludeFile('extern/*'), excludeFile('usr/*')],
                           skipBlames: true)
            }
          }

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
  return nodes
}

pipeline{
  agent none
  stages {
    stage("CI") {
      steps{
        script {
          def builds = jobMatrix('build', [
            [os: 'fedora', ver: '32',    arch: 'x86_64', compiler: 'gcc-10'],
            [os: 'macos',  ver: '11',    arch: 'x86_64', compiler: 'apple-clang-12'],
          ])

          parallel(builds)
        }
      }
    }
  }
}
