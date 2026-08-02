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
}

def runInContainer(command) {
    sh """docker run --rm -v \$WORKSPACE:/workspace -v vcpkg_cache:/root/.cache/vcpkg -w /workspace ${env.IMAGE} bash -c '
        ${command}
        code=\$?
        if [ \$code -ne 0 ]; then
            find /opt/vcpkg/buildtrees -name \"*.log\" -exec echo ==={}=== \\; -exec cat {} \\;
        fi
        exit \$code
    '"""
}
