pipeline {
  agent {
    docker {
      image 'gcc:5.4'
    }

  }
  stages {
    stage('Build') {
      steps {
        sh '''cd src
make -j4'''
      }
    }
    stage('Test') {
      steps {
        sh './src/gtest'
      }
    }
  }
}