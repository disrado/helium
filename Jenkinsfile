pipeline {
    agent none
    environment {
        IMAGE = 'helium-linux-build-env:latest'
    }
    stages {
        stage('Configure') {
            parallel {
                stage('Linux') {
                    agent { label 'linux' }
                    steps { script { runInContainer('cmake --preset linux-release') } }
                }
                stage('Windows') {
                    agent { label 'windows' }
                    steps { bat 'cmake --preset win-debug' }
                }
            }
        }
        stage('Build') {
            parallel {
                stage('Linux') {
                    agent { label 'linux' }
                    options { skipDefaultCheckout() }
                    steps { script { runInContainer('cmake --build build/linux-release') } }
                }
                stage('Windows') {
                    agent { label 'windows' }
                    options { skipDefaultCheckout() }
                    steps { bat 'cmake --build build/win-debug' }
                }
            }
        }
        stage('Test') {
            parallel {
                stage('Linux') {
                    agent { label 'linux' }
                    options { skipDefaultCheckout() }
                    steps {
                        script {
                            runInContainer('build/linux-release/engine/helium_test_suite')
                            runInContainer('build/linux-release/game/game_test_suite')
                        }
                    }
                    post { always { cleanWs() } }
                }
                stage('Windows') {
                    agent { label 'windows' }
                    options { skipDefaultCheckout() }
                    steps {
                        bat 'build\\win-debug\\engine\\helium_test_suite.exe'
                        bat 'build\\win-debug\\game\\game_test_suite.exe'
                    }
                    post { always { cleanWs() } }
                }
            }
        }
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
