pipeline {
    agent {
        docker {
            image 'helium-build-env:latest'
            label 'linux'
        }
    }
    stages {
        stage('Configure') {
            steps { sh 'cmake --preset linux-release' }
        }
        stage('Build') {
            steps { sh 'cmake --build build/linux-release' }
        }
        stage('Test') {
            steps {
                sh 'build/linux-release/engine/helium_test_suite'
                sh 'build/linux-release/game/game_test_suite'
            }
        }
    }
}
