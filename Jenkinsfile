pipeline {
  agent any
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