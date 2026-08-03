pipeline {
    agent { label 'linux' }
    environment {
        IMAGE = 'helium-linux-build-env:latest'
    }
    stages {
        stage('Configure') {
            steps { script { runInContainer('cmake --preset linux-release') } }
        }
        stage('Build') {
            steps { script { runInContainer('cmake --build build/linux-release') } }
        }
        stage('Test') {
            steps {
                script {
                    runInContainer('build/linux-release/engine/helium_test_suite')
                    runInContainer('build/linux-release/game/game_test_suite')
                }
            }
        }
    }
    post {
        always { cleanWs() }
    }
}

def runInContainer(command) {
    sh """docker run --rm -v \$WORKSPACE:/workspace -v vcpkg_cache:/root/.cache/vcpkg -w /workspace ${env.IMAGE} bash -c '
        dump_logs_on_failure() {
            if [ "\$1" -ne 0 ]; then
                find /opt/vcpkg/buildtrees -name \"*.log\" -exec echo ==={}=== \\; -exec cat {} \\;
            fi
        }

        ${command}
        code=\$?
        dump_logs_on_failure \$code
        exit \$code
    '"""
}
