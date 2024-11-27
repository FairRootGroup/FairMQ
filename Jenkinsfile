#!groovy

def jobMatrix(String type, List specs) {
  def nodes = [:]
  for (spec in specs) {
    def job = ""
    def selector = "slurm"
    def os = ""
    def ver = ""

    if (type == 'build') {
      job = "${spec.os}-${spec.ver}-${spec.arch}-${spec.compiler}"
      if (spec.os =~ /^macos/) {
        selector = "${spec.os}-${spec.ver}-${spec.arch}"
      }
      os = spec.os
      ver = spec.ver
    } else { // == 'check'
      job = "${spec.name}"
      os = 'fedora'
      ver = '36'
    }

    def label = "${job}"
    def extra = spec.extra

    nodes[label] = {
      node(selector) {
        githubNotify(context: "${label}", description: 'Building ...', status: 'PENDING')
        try {
          deleteDir()
          checkout scm

          def jobscript = 'job.sh'
          def ctestcmd = "ctest ${extra} -S FairMQTest.cmake -V --output-on-failure"
          sh "echo \"set -e\" >> ${jobscript}"
          sh "echo \"export LABEL=\\\"\${JOB_BASE_NAME} ${label}\\\"\" >> ${jobscript}"
          if (selector =~ /^macos/) {
            sh """\
              echo \"${ctestcmd}\" >> ${jobscript}
            """
            sh "cat ${jobscript}"
            sh "bash ${jobscript}"
          } else { // selector == "slurm"
            def imageurl = "oras://ghcr.io/fairrootgroup/fairmq-dev/${os}-${ver}-sif:latest"
            def execopts = "--ipc --uts --pid -B/shared"
            def containercmd = "singularity exec ${execopts} ${imageurl} bash -l -c \\\"${ctestcmd} ${extra}\\\""
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

            if (job == "static-analyzers") {
              recordIssues(enabledForFailure: true,
                           tools: [gcc(pattern: 'build/Testing/Temporary/*.log')],
                           filters: [excludeFile('extern/*'), excludeFile('usr/*')],
                           skipBlames: true,
                           skipPublishingChecks: true)
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
            [os: 'ubuntu', ver: '20.04', arch: 'x86_64', compiler: 'gcc-9'],
            [os: 'ubuntu', ver: '22.04', arch: 'x86_64', compiler: 'gcc-11'],
            [os: 'ubuntu', ver: '24.04', arch: 'x86_64', compiler: 'gcc-13'],
            [os: 'fedora', ver: '33',    arch: 'x86_64', compiler: 'gcc-10'],
            [os: 'fedora', ver: '34',    arch: 'x86_64', compiler: 'gcc-11'],
            [os: 'fedora', ver: '35',    arch: 'x86_64', compiler: 'gcc-11'],
            [os: 'fedora', ver: '36',    arch: 'x86_64', compiler: 'gcc-12'],
            [os: 'fedora', ver: '37',    arch: 'x86_64', compiler: 'gcc-12'],
            [os: 'fedora', ver: '38',    arch: 'x86_64', compiler: 'gcc-13'],
            [os: 'fedora', ver: '39',    arch: 'x86_64', compiler: 'gcc-13'],
            [os: 'fedora', ver: '40',    arch: 'x86_64', compiler: 'gcc-14'],
            [os: 'macos',  ver: '14',    arch: 'x86_64', compiler: 'apple-clang-16'],
            [os: 'macos',  ver: '15',    arch: 'x86_64', compiler: 'apple-clang-16'],
            [os: 'macos',  ver: '15',    arch: 'arm64',  compiler: 'apple-clang-16'],
          ])

          def all_debug = "-DCMAKE_BUILD_TYPE=Debug"

          def checks = jobMatrix('check', [
            [name: 'static-analyzers', extra: "${all_debug} -DRUN_STATIC_ANALYSIS=ON"],
            [name: '{address,leak,ub}-sanitizers',
             extra: "${all_debug} -DENABLE_SANITIZER_ADDRESS=ON -DENABLE_SANITIZER_LEAK=ON -DENABLE_SANITIZER_UNDEFINED_BEHAVIOUR=ON -DCMAKE_CXX_FLAGS='-O1 -fno-omit-frame-pointer'"],
            [name: 'thread-sanitizer', extra: "${all_debug} -DENABLE_SANITIZER_THREAD=ON -DCMAKE_CXX_COMPILER=clang++"],
          ])

          parallel(builds + checks)
        }
      }
    }
  }
}
