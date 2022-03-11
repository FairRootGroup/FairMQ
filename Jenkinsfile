#!groovy

def jobMatrix(String type, List specs) {
  def nodes = [:]
  for (spec in specs) {
    def job = ""
    def selector = ""
    def os = ""
    def ver = ""

    if (type == 'build') {
      job = "${spec.os}-${spec.ver}-${spec.arch}-${spec.compiler}"
      selector = "${spec.os}-${spec.ver}-${spec.arch}"
      os = spec.os
      ver = spec.ver
    } else { // == 'check'
      job = "${spec.name}"
      selector = 'fedora-35-x86_64'
      os = 'fedora'
      ver = '35'
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
              echo \"export DDS_ROOT=\\\"\\\$(brew --prefix dds)\\\"\" >> ${jobscript}
              echo \"export PATH=\\\"\\\$(brew --prefix dds)/bin:\\\$PATH\\\"\" >> ${jobscript}
              echo \"${ctestcmd}\" >> ${jobscript}
            """
            sh "cat ${jobscript}"
            sh "bash ${jobscript}"
          } else {
            def containercmd = "singularity exec --net --ipc --uts --pid -B/shared ${env.SINGULARITY_CONTAINER_ROOT}/fairmq/${os}.${ver}.sif bash -l -c \\\"${ctestcmd} ${extra}\\\""
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
          def tarball = "${type}_${job}_dds_logs.tar.gz"
          if (fileExists("build/test/.DDS")) {
            sh "tar czvf ${tarball} -C \${WORKSPACE}/build/test .DDS/"
            archiveArtifacts tarball
          }

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
          def all = '-DHAS_ASIO=ON -DHAS_ASIOFI=ON -DHAS_PMIX=ON'

          def builds = jobMatrix('build', [
            [os: 'ubuntu', ver: '20.04', arch: 'x86_64', compiler: 'gcc-9',  extra: all],
            [os: 'fedora', ver: '32',    arch: 'x86_64', compiler: 'gcc-10', extra: all],
            [os: 'fedora', ver: '33',    arch: 'x86_64', compiler: 'gcc-10', extra: all],
            [os: 'fedora', ver: '34',    arch: 'x86_64', compiler: 'gcc-11', extra: all],
            [os: 'fedora', ver: '35',    arch: 'x86_64', compiler: 'gcc-11', extra: all],
            [os: 'macos',  ver: '11',    arch: 'x86_64', compiler: 'apple-clang-12', extra: '-DHAS_ASIO=ON'],
            [os: 'macos',  ver: '11',    arch: 'arm64', compiler: 'apple-clang-13', extra: '-DHAS_ASIO=ON'],
          ])

          def all_debug = "${all} -DCMAKE_BUILD_TYPE=Debug"

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
